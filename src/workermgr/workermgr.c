#include "workermgr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pcap.h>
#include <sys/prctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/syslog.h>

#include "algorithms/queue/queue.h"
#include "algorithms/telemetry/telemetry.h"
#include "algorithms/timer/timer.h"
#include "psmgr/psmgr.h"
#include "database/database.h"
#include "packet_analyze.h"

struct workermgr_pipeline_t
{
    queue_t *ring_buffer;
    telemetry_list_t *telemetry_list;
    telemetry_entry_t *stat_enq;
    telemetry_entry_t *stat_deq;
    telemetry_entry_t *stat_fail;
};

/* Auxiliary wrapper structures used to carry multiple arguments across process boundaries */
typedef struct
{
    workermgr_pipeline_t *pipeline;
    char target_name[256]; /* FIXED: Absolute safe bounded string size buffer layout array */
} worker_args_t;

// ====================================================================
// MASTER SERVICE INITIALIZATION SWEEPS
// ====================================================================

int workermgr_start(void)
{
    printf("[WORKERMGR] Core processing orchestration master unit started.\n");
    return 0;
}

void workermgr_stop(void)
{
    printf("[WORKERMGR] Stopping active queues channels and tearing down orchestration execution lines.\n");
}

workermgr_pipeline_t *workermgr_pipeline_create(const char *name, size_t queue_memory_size)
{
    if (name == NULL || queue_memory_size == 0)
        return NULL;

    workermgr_pipeline_t *pipe = (workermgr_pipeline_t *)malloc(sizeof(workermgr_pipeline_t));
    if (pipe == NULL)
        return NULL;

    pipe->ring_buffer = queue_create(queue_memory_size);
    if (pipe->ring_buffer == NULL)
    {
        free(pipe);
        return NULL;
    }

    char t_name[256];
    snprintf(t_name, sizeof(t_name), "PIPE_%s", name);
    pipe->telemetry_list = telemetry_list_create(t_name);

    if (pipe->telemetry_list != NULL)
    {
        pipe->stat_enq = (telemetry_entry_t *)malloc(sizeof(void *));
        pipe->stat_deq = (telemetry_entry_t *)malloc(sizeof(void *));
        pipe->stat_fail = (telemetry_entry_t *)malloc(sizeof(void *));

        telemetry_alloc(pipe->telemetry_list, "pipeline.produce.count", TELEMETRY_TYPE_SUM, pipe->stat_enq);
        telemetry_alloc(pipe->telemetry_list, "pipeline.consume.count", TELEMETRY_TYPE_SUM, pipe->stat_deq);
        telemetry_alloc(pipe->telemetry_list, "pipeline.dropped.count", TELEMETRY_TYPE_SUM, pipe->stat_fail);
    }

    return pipe;
}

void workermgr_pipeline_destroy(workermgr_pipeline_t *pipeline)
{
    if (pipeline == NULL)
        return;
    if (pipeline->ring_buffer)
        queue_destroy(pipeline->ring_buffer);
    if (pipeline->telemetry_list)
        telemetry_list_destroy(pipeline->telemetry_list);
    if (pipeline->stat_enq)
        free(pipeline->stat_enq);
    if (pipeline->stat_deq)
        free(pipeline->stat_deq);
    if (pipeline->stat_fail)
        free(pipeline->stat_fail);
    free(pipeline);
}

// ====================================================================
// ORCHESTRATION DISPATCHERS: FORK SPAWN SYSTEM HOOKS
// ====================================================================

int workermgr_spawn_producer(workermgr_pipeline_t *pipeline, const char *interface_name)
{
    if (pipeline == NULL || interface_name == NULL)
        return -1;

    printf("[WORKERMGR][MASTER] Attempting to spawn producer instance for interface: %s\n", interface_name);

    worker_args_t *args = (worker_args_t *)malloc(sizeof(worker_args_t));
    if (args == NULL)
        return -1;
    args->pipeline = pipeline;
    strncpy(args->target_name, interface_name, sizeof(args->target_name) - 1);
    args->target_name[sizeof(args->target_name) - 1] = '\0';

    char s_name[256], f_name[256];
    snprintf(s_name, sizeof(s_name), "prod_%s", interface_name);
    snprintf(f_name, sizeof(f_name), "Niuchacz PCAP Producer Process - %s", interface_name);

    return psmgr_create_process(s_name, f_name, PSMGR_PROC_USER, i_workermgr_producer_routine, args, NULL);
}

int workermgr_spawn_consumers(workermgr_pipeline_t *pipeline, const char *db_filename, int instances_count)
{
    if (pipeline == NULL || db_filename == NULL || instances_count <= 0)
        return -1;

    printf("[WORKERMGR][MASTER] Attempting to spawn %d consumer instances for database: %s\n", instances_count, db_filename);

    for (int i = 0; i < instances_count; i++)
    {
        worker_args_t *args = (worker_args_t *)malloc(sizeof(worker_args_t));
        if (args == NULL)
            return -1;
        args->pipeline = pipeline;

        strncpy(args->target_name, db_filename, sizeof(args->target_name) - 1);
        args->target_name[sizeof(args->target_name) - 1] = '\0';

        char s_name[256], f_name[256];
        snprintf(s_name, sizeof(s_name), "cons_%d", i);
        snprintf(f_name, sizeof(f_name), "Niuchacz Packet Consumer Analyzer Process ID: %d", i);

        printf("[WORKERMGR][MASTER] Requesting psmgr to fork child process allocation: %s\n", s_name);
        int ret = psmgr_create_process(s_name, f_name, PSMGR_PROC_USER, i_workermgr_consumer_routine, args, NULL);
        if (ret != 0)
        {
            fprintf(stderr, "[WORKERMGR][MASTER][ERROR] psmgr_create_process failed for %s with code: %d\n", s_name, ret);
            free(args);
            return ret;
        }
    }
    return 0;
}

// ====================================================================
// PRIVATE WORKER EXECUTION PATHS (FORKED CHILD OPERATIONS)
// ====================================================================

int i_workermgr_producer_routine(void *arg) {
    worker_args_t *args = (worker_args_t*)arg;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr header;
    const unsigned char *packet_bytes = NULL;
    pid_t my_pid = getpid();
    
    char filter_exp[] = "ip and not (dst net 192.168.0.0/16 and src net 192.168.0.0/16)";
    struct bpf_program fp;
    bpf_u_int32 net = 0, mask = 0;

    printf("[WORKERMGR][CHILD][PID:%d] Producer entry. Device: '%s', ring_buffer pointer: %p\n", 
           my_pid, args ? args->target_name : "NULL", (void*)(args ? args->pipeline->ring_buffer : NULL));

    if (args == NULL || args->pipeline == NULL || args->pipeline->ring_buffer == NULL) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] Producer started with NULL structures targets!\n", my_pid);
        if (args) free(args);
        return -1;
    }

    if (!queue_producer_new(args->pipeline->ring_buffer)) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] Producer pipeline registration claims rejected by queue context!\n", my_pid);
        free(args);
        return -1;
    }
    
    if (pcap_lookupnet(args->target_name, &net, &mask, errbuf) == -1) { net = 0; mask = 0; }
    pcap_t *pcap_handle = pcap_open_live(args->target_name, BUFSIZ, 1, 1000, errbuf);
    if (pcap_handle == NULL) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] libpcap pcap_open_live failed for interface '%s': %s\n", my_pid, args->target_name, errbuf);
        queue_producer_free(args->pipeline->ring_buffer);
        free(args);
        return -1;
    }

    if (pcap_datalink(pcap_handle) != DLT_EN10MB || pcap_compile(pcap_handle, &fp, filter_exp, 0, net) == -1 || pcap_setfilter(pcap_handle, &fp) == -1) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] Failed to configure link layout or BPF filter specs: %s\n", my_pid, pcap_geterr(pcap_handle));
        pcap_close(pcap_handle);
        queue_producer_free(args->pipeline->ring_buffer);
        free(args);
        return -1;
    }
    printf("[WORKERMGR][CHILD][PID:%d] BPF filter compiled. Starting active capture wire loop...\n", my_pid);

    while (psmgr_is_running()) {
        packet_bytes = pcap_next(pcap_handle, &header);
        if (packet_bytes != NULL) {
            size_t payload_size = header.caplen;
            size_t allocation_chunk_size = sizeof(worker_packet_header_t) + payload_size;
            
            void *marshalling_buffer = malloc(allocation_chunk_size);
            if (marshalling_buffer == NULL) {
                telemetry_update(args->pipeline->stat_fail, 1);
                continue;
            }

            worker_packet_header_t *p_meta = (worker_packet_header_t*)marshalling_buffer;
            p_meta->payload_size = payload_size;
            p_meta->timestamp = header.ts;
            strncpy(p_meta->interface_name, args->target_name, sizeof(p_meta->interface_name) - 1);
            p_meta->interface_name[sizeof(p_meta->interface_name) - 1] = '\0';

            void *p_payload_target = (void*)((uintptr_t)marshalling_buffer + sizeof(worker_packet_header_t));
            memcpy(p_payload_target, packet_bytes, payload_size);

            int queue_status = queue_write(args->pipeline->ring_buffer, marshalling_buffer, allocation_chunk_size, NULL);
            free(marshalling_buffer);

            if (queue_status > 0) {
                telemetry_update(args->pipeline->stat_enq, 1);
            } else {
                /* ADDED LOGGING: Capture packet insertion drops natively */
                fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][WARN] queue_write dropped packet block! Status code return: %d\n", my_pid, queue_status);
                telemetry_update(args->pipeline->stat_fail, 1);
            }
        }
    }

    fprintf(stderr, "[WORKERMGR][CHILD][PID:%d] Producer loop broken. Closing capture context handle.\n", my_pid);
    queue_producer_free(args->pipeline->ring_buffer);
    pcap_freecode(&fp);
    pcap_close(pcap_handle);
    free(args);
    return 0;
}

int i_workermgr_consumer_routine(void *arg) {
    worker_args_t *args = (worker_args_t*)arg;
    database_t *db_handle = NULL;
    pid_t my_pid = getpid();
    
    printf("[WORKERMGR][CHILD][PID:%d] Consumer process entry. Checking internal pointers: args=%p, pipeline=%p\n", 
           my_pid, (void*)args, (void*)(args ? args->pipeline : NULL));

    if (args == NULL || args->pipeline == NULL || args->pipeline->ring_buffer == NULL) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] Critical structural fault: passed pointers are NULL!\n", my_pid);
        if (args) free(args);
        return -1;
    }

    int db_status = db_open("DB_CONSUMER", args->target_name, &db_handle);
    if (db_status != 0) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] db_open failed with error code: %d for file: '%s'.\n", 
                my_pid, db_status, args->target_name);
        free(args);
        return -1;
    }

    void *buffer = malloc(1024 * 1024);
    if (buffer == NULL) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] Local 1MB scratchpad buffer allocation failed!\n", my_pid);
        db_close(db_handle);
        free(args);
        return -1;
    }

    if (!queue_consumer_new(args->pipeline->ring_buffer)) {
        fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] Shared pipeline registration rejected!\n", my_pid);
        free(buffer);
        db_close(db_handle);
        free(args);
        return -1;
    }
    
    printf("[WORKERMGR][CHILD][PID:%d] Attached to ring-buffer. Entering active loop...\n", my_pid);
    struct timespec timeout = { .tv_sec = 0, .tv_nsec = 100000000 }; /* 100ms */

    while (psmgr_is_running()) {
        /* ADDED LOGGING: Debug log before every read attempt */
        // printf("[WORKERMGR][CHILD][PID:%d][DEBUG] Calling queue_read...\n", my_pid);
        
        int read_bytes = queue_read(args->pipeline->ring_buffer, buffer, &timeout);
        
        /* ADDED LOGGING: Detailed status evaluation after read action */
        if (read_bytes > 0) {
            worker_packet_header_t *p_meta = (worker_packet_header_t*)buffer;
            void *p_payload = (void*)((uintptr_t)buffer + sizeof(worker_packet_header_t));

            db_txn_begin(db_handle);
            int analyze_status = cmdPacketAnalyzeExec((void*)db_handle, p_meta->timestamp, p_payload, p_meta->payload_size);
            
            if (analyze_status == 0) {
                db_txn_commit(db_handle);
                telemetry_update(args->pipeline->stat_deq, 1);
            } else {
                db_txn_rollback(db_handle);
                telemetry_update(args->pipeline->stat_fail, 1);
                fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][WARN] cmdPacketAnalyzeExec rejected package frame, code: %d\n", my_pid, analyze_status);
            }
        } else {
            /* ADDED LOGGING: Log non-positive outcomes specifically */
            if (read_bytes == QUEUE_RET_DESTROYING) {
                fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][FATAL] queue_read returned QUEUE_RET_DESTROYING! The queue was invalidated.\n", my_pid);
                break;
            } else if (read_bytes == QUEUE_RET_ERROR) {
                fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][ERROR] queue_read returned QUEUE_RET_ERROR! Data corruption or checksum failure.\n", my_pid);
                break;
            } else if (read_bytes == QUEUE_RET_TIMEOUT) {
                /* Timeout is normal when there is no traffic, we log it only as trace if needed */
                // printf("[WORKERMGR][CHILD][PID:%d][TRACE] queue_read timeout (no packets available)\n", my_pid);
            } else {
                fprintf(stderr, "[WORKERMGR][CHILD][PID:%d][UNKNOWN] queue_read returned undocumented code: %d\n", my_pid, read_bytes);
            }
        }
    }

    fprintf(stderr, "[WORKERMGR][CHILD][PID:%d] Exited main loop. Reason: psmgr_is_running=%s\n", my_pid, psmgr_is_running() ? "true" : "false");
    queue_consumer_free(args->pipeline->ring_buffer);
    free(buffer);
    db_close(db_handle);
    free(args);
    return 0;
}
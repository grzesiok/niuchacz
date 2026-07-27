#include "workermgr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pcap.h>
#include <sys/prctl.h>
#include <signal.h>

#include "algorithms/queue/queue.h"
#include "algorithms/telemetry/telemetry.h"
#include "algorithms/timer/timer.h"
#include "psmgr/psmgr.h"
#include "database/database.h"
#include "packet_analyze.h"

struct workermgr_pipeline_t {
    queue_t *ring_buffer;
    telemetry_list_t *telemetry_list;
    telemetry_entry_t *stat_enq;
    telemetry_entry_t *stat_deq;
    telemetry_entry_t *stat_fail;
};

/* Auxiliary wrapper structures used to carry multiple arguments across process boundaries */
typedef struct {
    workermgr_pipeline_t *pipeline;
    char target_name[256]; /* FIXED: Replaced raw char scalar with standard string bounds length */
} worker_args_t;

// ====================================================================
// MASTER SERVICE INITIALIZATION Sweeps
// ====================================================================

int workermgr_start(void) {
    printf("[WORKERMGR] Core processing orchestration master unit started.\n");
    return 0;
}

void workermgr_stop(void) {
    printf("[WORKERMGR] Stopping active queues channels and tearing down orchestration execution lines.\n");
}

workermgr_pipeline_t* workermgr_pipeline_create(const char *name, size_t queue_memory_size) {
    if (name == NULL || queue_memory_size == 0) return NULL;

    workermgr_pipeline_t *pipe = (workermgr_pipeline_t*)malloc(sizeof(workermgr_pipeline_t));
    if (pipe == NULL) return NULL;

    pipe->ring_buffer = queue_create(queue_memory_size);
    if (pipe->ring_buffer == NULL) {
        free(pipe);
        return NULL;
    }

    char t_name[64]; /* FIXED: Adjusted string buffer allocation boundaries */
    snprintf(t_name, sizeof(t_name), "PIPE_%s", name);
    pipe->telemetry_list = telemetry_list_create(t_name);
    
    if (pipe->telemetry_list != NULL) {
        pipe->stat_enq  = (telemetry_entry_t*)malloc(sizeof(void*));
        pipe->stat_deq  = (telemetry_entry_t*)malloc(sizeof(void*));
        pipe->stat_fail = (telemetry_entry_t*)malloc(sizeof(void*));
        
        telemetry_alloc(pipe->telemetry_list, "pipeline.produce.count", TELEMETRY_TYPE_SUM, pipe->stat_enq);
        telemetry_alloc(pipe->telemetry_list, "pipeline.consume.count", TELEMETRY_TYPE_SUM, pipe->stat_deq);
        telemetry_alloc(pipe->telemetry_list, "pipeline.dropped.count", TELEMETRY_TYPE_SUM, pipe->stat_fail);
    }

    return pipe;
}

void workermgr_pipeline_destroy(workermgr_pipeline_t *pipeline) {
    if (pipeline == NULL) return;
    if (pipeline->ring_buffer) queue_destroy(pipeline->ring_buffer);
    if (pipeline->telemetry_list) telemetry_list_destroy(pipeline->telemetry_list);
    if (pipeline->stat_enq) free(pipeline->stat_enq);
    if (pipeline->stat_deq) free(pipeline->stat_deq);
    if (pipeline->stat_fail) free(pipeline->stat_fail);
    free(pipeline);
}

// ====================================================================
// ORCHESTRATION DISPATCHERS: FORK SPAWN SYSTEM HOOKS
// ====================================================================

int workermgr_spawn_producer(workermgr_pipeline_t *pipeline, const char *interface_name) {
    if (pipeline == NULL || interface_name == NULL) return -1;

    worker_args_t *args = (worker_args_t*)malloc(sizeof(worker_args_t));
    if (args == NULL) return -1;
    args->pipeline = pipeline;
    strncpy(args->target_name, interface_name, sizeof(args->target_name) - 1);
    args->target_name[sizeof(args->target_name) - 1] = '\0';

    char s_name[64], f_name[256]; /* FIXED: Upgraded from raw char to standard buffer lengths sizes */
    snprintf(s_name, sizeof(s_name), "prod_%s", interface_name);
    snprintf(f_name, sizeof(f_name), "Niuchacz PCAP Producer Process - %s", interface_name);

    /* FIXED: Appended the 6th parameter matching out_pid descriptor requirements */
    return psmgr_create_process(s_name, f_name, PSMGR_PROC_USER, i_workermgr_producer_routine, args, NULL);
}

int workermgr_spawn_consumers(workermgr_pipeline_t *pipeline, const char *db_filename, int instances_count) {
    if (pipeline == NULL || db_filename == NULL || instances_count <= 0) return -1;

    for (int i = 0; i < instances_count; i++) {
        worker_args_t *args = (worker_args_t*)malloc(sizeof(worker_args_t));
        if (args == NULL) return -1;
        args->pipeline = pipeline;
        strncpy(args->target_name, db_filename, sizeof(args->target_name) - 1);
        args->target_name[sizeof(args->target_name) - 1] = '\0';

        char s_name[64], f_name[256]; /* FIXED: Upgraded from raw char to standard buffer lengths sizes */
        snprintf(s_name, sizeof(s_name), "cons_%d", i);
        snprintf(f_name, sizeof(f_name), "Niuchacz Packet Consumer Analyzer Process ID: %d", i);

        /* FIXED: Appended the 6th parameter matching out_pid descriptor requirements */
        int ret = psmgr_create_process(s_name, f_name, PSMGR_PROC_USER, i_workermgr_consumer_routine, args, NULL);
        if (ret != 0) return ret;
    }
    return 0;
}

// ====================================================================
// PRIVATE WORKER EXECUTION PATHS (EXECUTED INSIDE FORKED CHILD ZONE)
// ====================================================================

int i_workermgr_producer_routine(void *arg) {
    worker_args_t *args = (worker_args_t*)arg;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr header;
    const unsigned char *packet_bytes = NULL;
    
    char filter_exp[] = "ip and not (dst net 192.168.0.0/16 and src net 192.168.0.0/16)";
    struct bpf_program fp;
    bpf_u_int32 net = 0, mask = 0;

    queue_producer_new(args->pipeline->ring_buffer);
    
    if (pcap_lookupnet(args->target_name, &net, &mask, errbuf) == -1) { net = 0; mask = 0; }
    pcap_t *pcap_handle = pcap_open_live(args->target_name, BUFSIZ, 1, 1000, errbuf);
    if (pcap_handle == NULL) return -1;

    if (pcap_datalink(pcap_handle) != DLT_EN10MB || pcap_compile(pcap_handle, &fp, filter_exp, 0, net) == -1 || pcap_setfilter(pcap_handle, &fp) == -1) {
        pcap_close(pcap_handle);
        return -1;
    }

    while (1) {
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
                telemetry_update(args->pipeline->stat_fail, 1);
            }
        }
    }

    queue_producer_free(args->pipeline->ring_buffer);
    pcap_freecode(&fp);
    pcap_close(pcap_handle);
    free(args);
    return 0;
}

int i_workermgr_consumer_routine(void *arg) {
    worker_args_t *args = (worker_args_t*)arg;
    database_t *db_handle = NULL;
    
    int db_status = db_open("DB_CONSUMER", args->target_name, &db_handle);
    if (db_status != 0) {
        free(args);
        return -1;
    }

    void *buffer = malloc(1024 * 1024);
    if (buffer == NULL) {
        db_close(db_handle);
        free(args);
        return -1;
    }

    queue_consumer_new(args->pipeline->ring_buffer);
    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };

    while (1) {
        int read_bytes = queue_read(args->pipeline->ring_buffer, buffer, &timeout);
        
        if (read_bytes > 0) {
            worker_packet_header_t *p_meta = (worker_packet_header_t*)buffer;
            void *p_payload = (void*)((uintptr_t)buffer + sizeof(worker_packet_header_t));

            db_txn_begin(db_handle);
            int analyze_status = cmdPacketAnalyzeExec(p_meta->timestamp, p_payload, p_meta->payload_size);
            
            if (analyze_status == 0) {
                db_txn_commit(db_handle);
                telemetry_update(args->pipeline->stat_deq, 1);
            } else {
                db_txn_rollback(db_handle);
                telemetry_update(args->pipeline->stat_fail, 1);
            }
        } else if (read_bytes == QUEUE_RET_DESTROYING) {
            break;
        }
    }

    queue_consumer_free(args->pipeline->ring_buffer);
    free(buffer);
    db_close(db_handle);
    free(args);
    return 0;
}
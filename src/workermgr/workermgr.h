#ifndef _WORKER_MGR_H
#define _WORKER_MGR_H

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

/* Opaque pointer mapping representing the shared pipeline memory channel */
typedef struct workermgr_pipeline_t workermgr_pipeline_t;

/* Blueprint structure passed down to encapsulate packet data frames serialized into the queue */
typedef struct {
    size_t payload_size;
    struct timeval timestamp;
    char interface_name[16];
} worker_packet_header_t;

/* Public API Management Lifecycles (Returns 0 on success, negative on error) */
int workermgr_start(void);
void workermgr_stop(void);

/* Public API Pipeline Channel Management Factories */
workermgr_pipeline_t* workermgr_pipeline_create(const char *name, size_t queue_memory_size);
void workermgr_pipeline_destroy(workermgr_pipeline_t *pipeline);

/* Public API Orchestration Handlers to Spawn Producers and Consumers */
int workermgr_spawn_producer(workermgr_pipeline_t *pipeline, const char *interface_name);
int workermgr_spawn_consumers(workermgr_pipeline_t *pipeline, const char *db_filename, int instances_count);

/* Public Under-the-hood Worker Routine Injections (Executed inside forked processes) */
int i_workermgr_producer_routine(void *arg);
int i_workermgr_consumer_routine(void *arg);

#endif /* _WORKER_MGR_H */
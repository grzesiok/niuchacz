/*
 * workmgr.h
 * Work Manager Interface (ANSI C)
 * Bridges dynamic execution logic with the process supervisor (psmgr).
 */

#ifndef WORKMGR_H
#define WORKMGR_H

#include "algorithms/queue/queue.h"

#define WORKMGR_SUCCESS  0
#define WORKMGR_ERROR   -1

/* 
 * Universal blueprint definition for any Work unit.
 * Isolates execution logic via clean function pointers.
 */
typedef struct {
    const char *work_type_name;         /* e.g., "PCAP_PRODUCER", "SQLITE_CONSUMER" */
    void (*work_setup)(void *ctx);      /* Invoked immediately after fork inside child context */
    void (*work_run_loop)(void *ctx);   /* The long-running operational core execution loop */
    void (*work_teardown)(void *ctx);   /* Invoked during graceful child process exit sequence */
} WorkDescriptor_t;

/* 
 * Internal runtime context bundled and passed to each spawned child process instance.
 */
typedef struct {
    queue_t *pipeline;                  /* Reference pointer to the process-shared queue */
    void *custom_config;                /* Reference to runtime configuration blocks (e.g., interface string) */
} WorkContext_t;

/* Initializes the Work Manager engine components */
int workmgr_init(void);

/* 
 * Dispatches a specified Work configuration type.
 * Spawns 'instance_count' parallel processes isolated via psmgr.
 */
int workmgr_start_work(const WorkDescriptor_t *work_item, queue_t *pipeline, void *custom_config, int instance_count);

/* Reclaims local operational memory allocations */
void workmgr_destroy(void);

#endif /* WORKMGR_H */
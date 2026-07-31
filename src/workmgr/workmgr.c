/*
 * workmgr.c
 * Work Manager Implementation (ANSI C)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "workmgr.h"
#include "psmgr/psmgr.h"

/* 
 * Internal dynamic memory tracker to clean up instantiated execution 
 * contexts when the master node shuts down.
 */
typedef struct ContextNode {
    WorkContext_t *context;
    struct ContextNode *next;
} ContextNode_t;

static ContextNode_t *g_context_list_head = NULL;

/* 
 * Private wrapper function executed natively inside the forked child process context.
 * Guarantees standard initialization and clean teardown execution frames.
 */
static void i_workmgr_child_wrapper(void *raw_ctx) {
    WorkContext_t *w_ctx = (WorkContext_t *)raw_ctx;
    
    /* Safely extract the original WorkDescriptor structure appended during layout initialization */
    /* We temporarily leverage custom_config tracking fields if needed or a dedicated wrapper struct */
    /* To keep it clean and robust, we expect the context to be unpacked safely */
    
    /* 
     * Note: Because this function executes post-fork inside the isolated child,
     * memory tracking maps are independent. The worker loop executes until 
     * a signal interrupts the blocking calls inside queue.c or psmgr triggers.
     */
    
    /* 
     * Finding the job blueprint to run. For standard setups, the parent passes 
     * a combined execution payload. Let's process the work directly using 
     * structural boundaries.
     */
}

/* 
 * Enhanced inner wrapper tracking execution schemas across the process mapping boundary.
 */
typedef struct {
    const WorkDescriptor_t *blueprint;
    WorkContext_t *work_ctx;
} ChildExecutionPayload_t;

static void i_workmgr_execution_gate(void *raw_payload) {
    ChildExecutionPayload_t *payload = (ChildExecutionPayload_t *)raw_payload;
    const WorkDescriptor_t *work = payload->blueprint;
    WorkContext_t *ctx = payload->work_ctx;

    if (!work || !ctx) {
        exit(EXIT_FAILURE);
    }

    /* 1. Invoke child context specific setups (e.g. open database, bind socket) */
    if (work->work_setup) {
        work->work_setup(ctx);
    }

    /* 2. Execute the primary long-running data loop */
    if (work->work_run_loop) {
        work->work_run_loop(ctx);
    }

    /* 3. Execute graceful local cleanups if the loop terminates cleanly */
    if (work->work_teardown) {
        work->work_teardown(ctx);
    }
}

int workmgr_init(void) {
    g_context_list_head = NULL;
    return WORKMGR_SUCCESS;
}

int workmgr_start_work(const WorkDescriptor_t *work_item, queue_t *pipeline, void *custom_config, int instance_count) {
    int i;
    if (!work_item || !pipeline || instance_count <= 0) return WORKMGR_ERROR;

    printf("[WORKMGR] Dispatching work type '%s' across %d execution nodes...\n", 
           work_item->work_type_name, instance_count);

    for (i = 0; i < instance_count; i++) {
        /* Allocate a robust context tracking packet for each individual worker process instance */
        WorkContext_t *ctx = (WorkContext_t *)malloc(sizeof(WorkContext_t));
        ChildExecutionPayload_t *payload = (ChildExecutionPayload_t *)malloc(sizeof(ChildExecutionPayload_t));
        
        if (!ctx || !payload) {
            fprintf(stderr, "[WORKMGR][ERROR] Execution framework memory allocation allocation failure.\n");
            if (ctx) free(ctx);
            if (payload) free(payload);
            return WORKMGR_ERROR;
        }

        ctx->pipeline = pipeline;
        ctx->custom_config = custom_config;

        payload->blueprint = work_item;
        payload->work_ctx = ctx;

        /* Track allocations inside the master node registry loop to guarantee cleanup paths */
        ContextNode_t *node = (ContextNode_t *)malloc(sizeof(ContextNode_t));
        if (node) {
            node->context = ctx;
            node->next = g_context_list_head;
            g_context_list_head = node;
        }

        /* 
         * Delegate the execution tracking layout to the Process Supervisor.
         * The supervisor executes fork() and diverts the execution line into the wrapper gate.
         */
        pid_t spawned_pid = psmgr_spawn_worker(work_item->work_type_name, i_workmgr_execution_gate, (void *)payload);
        
        /* The payload configuration block is now safely inherited by the child, master can release its temporary copy */
        free(payload); 

        if (spawned_pid < 0) {
            fprintf(stderr, "[WORKMGR][ERROR] Failed to deploy instance %d of work layout '%s'.\n", i, work_item->work_type_name);
            return WORKMGR_ERROR;
        }
    }

    return WORKMGR_SUCCESS;
}

void workmgr_destroy(void) {
    ContextNode_t *current = g_context_list_head;
    while (current != NULL) {
        ContextNode_t *next_node = current->next;
        if (current->context) {
            free(current->context);
        }
        free(current);
        current = next_node;
    }
    g_context_list_head = NULL;
    printf("[WORKMGR] Execution engine tracking structures cleared successfully.\n");
}
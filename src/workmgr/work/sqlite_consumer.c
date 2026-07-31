/*
 * src/work/sqlite_consumer.c
 * SQLite Database Pipeline Consumer Work Implementation (ANSI C)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "workmgr/workmgr.h"
#include "algorithms/queue/queue.h"

#define SQLITE_CONSUMER_BUFFER_SIZE 65535

static void i_sqlite_consumer_setup(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    
    if (!ctx || !ctx->custom_config) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Database filename parameters are missing.\n");
        exit(EXIT_FAILURE);
    }

    const char *db_filename = (const char *)ctx->custom_config;
    printf("[SQLITE_CONSUMER] Initializing worker node bound to storage backend: %s\n", db_filename);

    /* Increment consumer metrics registration maps across the active shared queue */
    queue_consumer_new(ctx->pipeline);
}

static void i_sqlite_consumer_run_loop(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    char *local_flush_buffer = (char *)malloc(SQLITE_CONSUMER_BUFFER_SIZE);
    
    if (!local_flush_buffer) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Dynamic heap allocation bounds exceeded.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        /* Read packets out of the process-shared queue segment (blocks while empty) */
        int read_bytes = queue_read(ctx->pipeline, local_flush_buffer, NULL);

        if (read_bytes == QUEUE_RET_DESTROYING) {
            printf("[SQLITE_CONSUMER] Termination requested by parent node. Retracting worker allocation scopes.\n");
            break;
        }

        if (read_bytes > 0) {
            /* 
             * Place packet processing or raw database insertions here:
             * e.g., sqlite3_exec(...) or packet_analyze(local_flush_buffer, read_bytes);
             */
        }
    }

    free(local_flush_buffer);
}

static void i_sqlite_consumer_teardown(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    if (!ctx) return;

    printf("[SQLITE_CONSUMER] Database pipeline workspace detached safely.\n");
    queue_consumer_free(ctx->pipeline);
}

/* Public global registration descriptor reference used inside main.c */
WorkDescriptor_t sqlite_consumer_work = {
    .work_type_name = "SQLITE_CONSUMER",
    .work_setup = i_sqlite_consumer_setup,
    .work_run_loop = i_sqlite_consumer_run_loop,
    .work_teardown = i_sqlite_consumer_teardown
};
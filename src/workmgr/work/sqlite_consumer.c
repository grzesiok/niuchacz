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
#include <libconfig.h>
#include "database/database.h"

#define SQLITE_CONSUMER_BUFFER_SIZE 65535

static void i_sqlite_consumer_setup(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    if (!ctx || !ctx->custom_config) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Work configuration section missing.\n");
        exit(EXIT_FAILURE);
    }

    /* Expect custom_config to be a config_setting_t* pointing to works.sqlite_consumer */
    config_setting_t *setting = (config_setting_t *)ctx->custom_config;
    const char *db_filename = NULL;
    if (config_setting_lookup_string(setting, "database_file", &db_filename) == CONFIG_FALSE || db_filename == NULL || db_filename[0] == '\0') {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] 'database_file' not set in work config.\n");
        exit(EXIT_FAILURE);
    }

    printf("[SQLITE_CONSUMER] Initializing worker node bound to storage backend: %s\n", db_filename);

    /* Open database and store handle in local state */
    database_t *db = NULL;
    if (db_open("SQLC", db_filename, &db) != 0) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Failed to open database '%s'.\n", db_filename);
        exit(EXIT_FAILURE);
    }

    /* Replace custom_config with our local db handle state for the child */
    ctx->custom_config = (void *)db;

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
    /* Close and free database handle if present */
    database_t *db = (database_t *)ctx->custom_config;
    if (db) db_close(db);
    queue_consumer_free(ctx->pipeline);
}

/* Public global registration descriptor reference used inside main.c */
WorkDescriptor_t sqlite_consumer_work = {
    .work_type_name = "SQLITE_CONSUMER",
    .config_name = "sqlite_consumer",
    .work_setup = i_sqlite_consumer_setup,
    .work_run_loop = i_sqlite_consumer_run_loop,
    .work_teardown = i_sqlite_consumer_teardown
};
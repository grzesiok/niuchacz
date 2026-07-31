/*
 * main.c
 * Core daemon entry point for the multi-process execution framework.
 * Orchestrated around Process Manager (psmgr), Work Manager (workmgr), and Shared IPC Queue.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libconfig.h>
#include <sys/types.h>

/* Modular project architecture headers */
#include "psmgr/psmgr.h"
#include "workmgr/workmgr.h"
#include "algorithms/queue/queue.h"
#include "algorithms/telemetry/telemetry.h"

/* Reference to external functional Work definitions implemented across project modules */
extern WorkDescriptor_t pcap_producer_work;
extern WorkDescriptor_t sqlite_consumer_work;

/* Global atomic indicator tracking the primary daemon run loop lifecycle */
static volatile sig_atomic_t g_service_active = 1;

/* Signal callback intercepts OS termination requests cleanly */
static void i_main_signal_handler(int signo) {
    (void)signo;
    g_service_active = 0;
}

int main(int argc, char *argv[]) {
    /* 
     * FIX 1: Disable output stream buffering entirely for stdout.
     * Guarantees standard printf() strings hit journalctl immediately, 
     * preventing log losses when terminated via SIGKILL.
     */
    setvbuf(stdout, NULL, _IONBF, 0);

    config_t cfg;
    queue_t *shared_pipeline = NULL;
    
    /* Configurable variables holding baseline worker limits */
    int min_workers = 2;
    int max_workers = 4;
    int target_consumers_count = 2;
    
    if (argc < 2) {
        fprintf(stderr, "FATAL: Configuration file argument path missing (argv).\n");
        return EXIT_FAILURE;
    }

    /* Initialize and read configuration layouts */
    config_init(&cfg);
    if (!config_read_file(&cfg, argv[1])) {
        fprintf(stderr, "FATAL: Configuration parsing failure at %s:%d - %s\n",
                config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    /* 
     * FIX 2: Set sa_flags to 0 instead of SA_RESTART.
     * Removing SA_RESTART prevents the kernel from auto-restarting blocked operations
     * (like pthread_cond_wait, sem_wait, sleep) when SIGTERM arrives, allowing the main loop to break.
     */
    struct sigaction sa = { .sa_handler = i_main_signal_handler, .sa_flags = 0 };
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    /* 1. Read layout properties from configuration */
    if (config_lookup_int(&cfg, "WORKERS.min_count", &min_workers) == CONFIG_FALSE) {
        min_workers = 2;
    }
    if (config_lookup_int(&cfg, "WORKERS.max_count", &max_workers) == CONFIG_FALSE) {
        max_workers = 4;
    }

    if (min_workers <= 0 || max_workers <= 0 || min_workers > max_workers) {
        fprintf(stderr, "WARNING: Configured WORKERS layout parameters are invalid (min:%d, max:%d). Using safe defaults (2).\n", 
                min_workers, max_workers);
        target_consumers_count = 2;
    } else {
        target_consumers_count = min_workers;
    }

    /* 2. Start global shared utility blocks (Telemetry, etc.) */
    if (telemetry_mgr_start() != 0) {
        fprintf(stderr, "FATAL: Critical system initialization failure on telemetry setups.\n");
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    /* 3. Initialize low-level sub-systems (Process Manager and Work Manager) */
    if (psmgr_init(32) != 0 || workmgr_init() != 0) {
        fprintf(stderr, "FATAL: Core tracking engine layout allocation failed.\n");
        goto __service_shutdown;
    }

    /* 
     * FIX 3: Initialize the shared queue BEFORE spawning worker processes.
     * The allocated memory region maps behind MAP_SHARED inside queue_create(),
     * allowing transparent inherited access across following process forks.
     */
    size_t ring_buffer_bytes = 512 * 1024 * 1024; /* 512 MB */
    shared_pipeline = queue_create(ring_buffer_bytes);
    if (shared_pipeline == NULL) {
        fprintf(stderr, "FATAL: Core process-shared ring buffer allocation failure.\n");
        goto __service_shutdown;
    }

    /* 4. Spawn Consumer Work instances using the configuration parameters */
    const char *db_file = "niuchacz_production.db";
    printf("[MAIN] Provisioning parallel consumers worker pool via Work Manager. Target count: %d\n", target_consumers_count);
    if (workmgr_start_work(&sqlite_consumer_work, shared_pipeline, (void*)db_file, target_consumers_count) != 0) {
        fprintf(stderr, "FATAL: Unable to provision SQLite consumer workflows.\n");
        goto __service_shutdown;
    }

    /* 5. Read interfaces array configuration block and spawn Producer Work instances */
    config_setting_t *interfaces_setting = config_lookup(&cfg, "niuchacz.interfaces");
    if (interfaces_setting == NULL) {
        fprintf(stderr, "[MAIN][ERROR] Required configuration path 'niuchacz.interfaces' missing.\n");
    } else {
        int num_interfaces = config_setting_length(interfaces_setting);
        int i;
        for (i = 0; i < num_interfaces; i++) {
            const char *if_name = config_setting_get_string_elem(interfaces_setting, i);
            if (if_name != NULL) {
                printf("[MAIN] Connecting capture line: Spawning dynamic Producer Work for interface: %s\n", if_name);
                /* Passes the unique interface identifier string alongside the shared queue reference pointer */
                int prod_status = workmgr_start_work(&pcap_producer_work, shared_pipeline, (void*)if_name, 1);
                if (prod_status != 0) {
                    fprintf(stderr, "[MAIN][WARN] Failed to spawn Producer Work instance for interface '%s'.\n", if_name);
                }
            }
        }
    }

    /* 6. Core service monitoring run loop */
    printf("[MAIN] Service loop transition into active running state finalized successfully.\n");
    
    while (g_service_active) {
        /* Non-blocking background health check checks for crashed child PIDs and reaps zombies */
        psmgr_periodic_check();
        
        /* Yield thread context safely to reduce processor spin loads */
        usleep(250000); 
    }

    /* 7. Graceful teardown phase */
    printf("[MAIN] Shutdown signal received. Commencing orderly execution termination...\n");

__service_shutdown:
    /* Orderly worker cleanup via escalative timed signaling routines (SIGTERM -> SIGKILL) */
    psmgr_stop_all_workers();
    
    /* Safely clear shared ring memory configurations */
    if (shared_pipeline) {
        printf("[MAIN] Reclaiming shared memory queue resources...\n");
        queue_destroy(shared_pipeline);
    }
    
    /* Free local component infrastructure assets */
    workmgr_destroy();
    telemetry_mgr_stop();
    config_destroy(&cfg);

    printf("[MAIN] Service execution cleanup sequence finalized successfully. Done.\n");
    return EXIT_SUCCESS;
}
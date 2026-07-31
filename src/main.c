#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libconfig.h>
#include <sys/types.h>

#include "psmgr/psmgr.h"
#include "algorithms/telemetry/telemetry.h"
#include "workermgr/workermgr.h"

/* Global tracking atomic indicator for the core service run loop lifecycle */
static volatile sig_atomic_t g_service_active = 1;

/* Signal callback wrapper to intercept OS daemon stop pending requests gracefully */
static void i_main_signal_handler(int signo) {
    (void)signo;
    g_service_active = 0;
}

int main(int argc, char *argv[]) {
    /* WYMUSZENIE BRAKU BUFOROWANIA DLA STDOUT */
    setvbuf(stdout, NULL, _IONBF, 0);
    
    config_t cfg;
    config_setting_t *interfaces_setting;
    workermgr_pipeline_t *shared_pipeline = NULL;
    int num_interfaces = 0;
    
    /* Variables to hold dynamic scalable worker constraints */
    int min_workers = 2; /* Safe internal default fallback values */
    int max_workers = 4;
    int target_consumers_spawn_count = 2;
    
    if (argc < 2) {
        fprintf(stderr, "FATAL: Configuration file argument path missing (argv).\n");
        return EXIT_FAILURE;
    }

    config_init(&cfg);
    if (!config_read_file(&cfg, argv[1])) {
        fprintf(stderr, "FATAL: Configuration parsing failure at %s:%d - %s\n",
                config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    struct sigaction sa = { .sa_handler = i_main_signal_handler, .sa_flags = SA_RESTART };
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    /* ------------------------------------------------------------ */
    /* 1. READ AND VALIDATE DYNAMIC WORKER POOL CONSTRAINTS         */
    /* ------------------------------------------------------------ */
    if (config_lookup_int(&cfg, "WORKERS.min_count", &min_workers) == CONFIG_FALSE) {
        min_workers = 2; /* Recover using default if missing */
    }
    if (config_lookup_int(&cfg, "WORKERS.max_count", &max_workers) == CONFIG_FALSE) {
        max_workers = 4;
    }

    /* Enforce strict logical health validations over the configurations bounds */
    if (min_workers <= 0 || max_workers <= 0 || min_workers > max_workers) {
        fprintf(stderr, "WARNING: Configized WORKERS layout parameters are invalid (min:%d, max:%d). Using safe defaults (2).\n", 
                min_workers, max_workers);
        target_consumers_spawn_count = 2;
    } else {
        /* 
         * Currently initializing the pipeline at baseline min bounds.
         * The workermgr scaling unit can scale up to max_workers dynamically later.
         */
        target_consumers_spawn_count = min_workers;
    }

    /* ------------------------------------------------------------ */
    /* 2. START KERNEL UTILITIES AND SERVICES INTERFACES            */
    /* ------------------------------------------------------------ */
    if (psmgr_start() != 0 || telemetry_mgr_start() != 0 || workermgr_start() != 0) {
        fprintf(stderr, "FATAL: Critical system initialization failure encountered on core layout setups.\n");
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    shared_pipeline = workermgr_pipeline_create("LIVE_TRAFFIC", 512 * 1024 * 1024);
    if (shared_pipeline == NULL) {
        fprintf(stderr, "FATAL: Core ring-buffer pipeline memory allocation failure.\n");
        goto __service_shutdown;
    }

    /* ------------------------------------------------------------ */
    /* 3. SPAWN SCALED CONSUMERS USING THE CONFIGURED TARGET COUNT   */
    /* ------------------------------------------------------------ */
    const char *db_file = "niuchacz_production.db";
    printf("[MAIN] Provisioning parallel consumers worker pool. Dynamic target count: %d\n", target_consumers_spawn_count);
    if (workermgr_spawn_consumers(shared_pipeline, db_file, target_consumers_spawn_count) != 0) {
        fprintf(stderr, "FATAL: Unable to provision parallel execution consumers pool.\n");
        goto __service_shutdown;
    }

    /* ------------------------------------------------------------ */
    /* 4. CONFIGURE AND DEPLOY MULTIPLE PCAP PRODUCERS VIA ARRAYS   */
    /* ------------------------------------------------------------ */
    interfaces_setting = config_lookup(&cfg, "niuchacz.interfaces");
    if (interfaces_setting == NULL) {
        fprintf(stderr, "[MAIN][ERROR] Required setting configuration 'niuchacz.interfaces' array lookup dropped.\n");
        /* Treat as non-fatal or handle safely without destroying active consumer loops unexpectedly */
    } else {
        num_interfaces = config_setting_length(interfaces_setting);
        for (int i = 0; i < num_interfaces; i++) {
            const char *if_name = config_setting_get_string_elem(interfaces_setting, i);
            if (if_name != NULL) {
                printf("[MAIN] Connecting capture lines: Spawning Producer process for interface: %s\n", if_name);
                int prod_status = workermgr_spawn_producer(shared_pipeline, if_name);
                if (prod_status != 0) {
                    fprintf(stderr, "[MAIN][WARN] Failed to spawn producer for interface '%s'. Continuing operations.\n", if_name);
                }
            }
        }
    }

    /* ------------------------------------------------------------ */
    /* 5. CORE SERVICE RUN LOOP: IDLE WAITING BARRIER SLEEP LOOPS   */
    /* ------------------------------------------------------------ */
    printf("[MAIN] Service loop transition into active running states finalized successfully.\n");
    
    /* 
     * FIXED: Explicitly guarantee the main orchestration thread stays alive 
     * and monitors the OS shutdown signals without dropping execution frames.
     */
    while (g_service_active) {
        psmgr_idle(1); /* Non-blocking background telemetry sweep logs maintenance */
    }

    /* ------------------------------------------------------------ */
    /* 6. CLEANUP: DE-PROVISION SUBPROCESSES AND FLUSH ALLOCATIONS   */
    /* ------------------------------------------------------------ */
    printf("[MAIN] Shutdown signal received. Evicting and stopping worker processes...\n");

__service_shutdown:
    /* FIXED: We only trigger pipeline destruction AFTER we request child terminations */
    psmgr_stop_user_processes();
    
    if (shared_pipeline) {
        printf("[MAIN] Reclaiming shared pipeline memory channels...\n");
        workermgr_pipeline_destroy(shared_pipeline);
    }
    
    workermgr_stop();
    telemetry_mgr_stop();
    psmgr_stop();
    config_destroy(&cfg);

    printf("[MAIN] Service execution cleanup sequence finalized successfully. Done.\n");
    return EXIT_SUCCESS;
}
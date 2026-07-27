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
    config_t cfg;
    config_setting_t *interfaces_setting;
    workermgr_pipeline_t *shared_pipeline = NULL;
    int num_interfaces = 0;
    
    if (argc < 2) {
        fprintf(stderr, "FATAL: Configuration file argument path missing (argv[1]).\n");
        return EXIT_FAILURE;
    }

    /* ------------------------------------------------------------ */
    /* 1. INITIALIZATION: CONFIGURATION AND CORE MANAGEMENT LAYERS   */
    /* ------------------------------------------------------------ */
    config_init(&cfg);
    /* FIXED: Passed the exact file path array scalar argv[1] to fix the compiler type mismatch */
    if (!config_read_file(&cfg, argv[1])) {
        fprintf(stderr, "FATAL: Configuration parsing failure at %s:%d - %s\n",
                config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    /* Set up operating system signal listeners for service container lifecycle management */
    struct sigaction sa = { .sa_handler = i_main_signal_handler, .sa_flags = SA_RESTART };
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    /* Initialize background process orchestration layer managers */
    if (psmgr_start() != 0) {
        fprintf(stderr, "FATAL: Process manager subsystem failed initialization.\n");
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    /* Initialize independent telemetry monitoring logging layers */
    if (telemetry_mgr_start() != 0) {
        fprintf(stderr, "FATAL: Global telemetry framework tracking layout allocation failure.\n");
        psmgr_stop();
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    if (workermgr_start() != 0) {
        fprintf(stderr, "FATAL: Worker Pool Manager layer context initialization aborted.\n");
        goto __service_shutdown;
    }

    /* ------------------------------------------------------------ */
    /* 2. INITIALIZE CENTRAL PROCESSING PIPELINE MEMORY CHANNELS    */
    /* ------------------------------------------------------------ */
    /* Allocating a single high-speed shared 512MB Ring Buffer for multiprocess messaging */
    shared_pipeline = workermgr_pipeline_create("LIVE_TRAFFIC", 512 * 1024 * 1024);
    if (shared_pipeline == NULL) {
        fprintf(stderr, "FATAL: Core ring-buffer pipeline memory allocation failure.\n");
        goto __service_shutdown;
    }

    /* ------------------------------------------------------------ */
    /* 3. CONFIGURE AND DEPLOY HIGH-SPEED MULTI-PROCESSED CONSUMERS */
    /* ------------------------------------------------------------ */
    /* Spawns a pool of 4 memory-isolated parallel consumers racing for packet extraction */
    const char *db_file = "niuchacz_production.db";
    if (workermgr_spawn_consumers(shared_pipeline, db_file, 4) != 0) {
        fprintf(stderr, "FATAL: Unable to provision parallel execution consumers pool.\n");
        goto __service_shutdown;
    }

    /* ------------------------------------------------------------ */
    /* 4. CONFIGURE AND DEPLOY MULTIPLE PCAP PRODUCERS VIA ARRAYS   */
    /* ------------------------------------------------------------ */
    interfaces_setting = config_lookup(&cfg, "niuchacz.interfaces");
    if (interfaces_setting == NULL) {
        fprintf(stderr, "FATAL: Required setting configuration 'niuchacz.interfaces' array lookup dropped.\n");
        goto __service_shutdown;
    }

    num_interfaces = config_setting_length(interfaces_setting);
    for (int i = 0; i < num_interfaces; i++) {
        const char *if_name = config_setting_get_string_elem(interfaces_setting, i);
        if (if_name != NULL) {
            printf("[MAIN] Connecting capture lines: Spawning Producer process for interface: %s\n", if_name);
            workermgr_spawn_producer(shared_pipeline, if_name);
        }
    }

    /* ------------------------------------------------------------ */
    /* 5. CORE SERVICE RUN LOOP: IDLE WAITING BARRIER SLEEP LOOPS   */
    /* ------------------------------------------------------------ */
    printf("[MAIN] Service loop transition into active running states finalized successfully.\n");
    while (g_service_active) {
        /* Non-blocking background telemetry sweep logs maintenance */
        psmgr_idle(1); 
    }

    /* ------------------------------------------------------------ */
    /* 6. CLEANUP: DE-PROVISION SUBPROCESSES AND FLUSH ALLOCATIONS   */
    /* ------------------------------------------------------------ */
    printf("[MAIN] Shutdown signal received. Evicting and stopping worker processes...\n");

__service_shutdown:
    if (shared_pipeline) {
        workermgr_pipeline_destroy(shared_pipeline);
    }
    /* Blocks and waits systematically for all active user processes to finalize exits loops */
    psmgr_stop_user_processes();
    
    /* Clean storage descriptors metrics allocation arrays */
    workermgr_stop();
    telemetry_mgr_stop();
    psmgr_stop();
    config_destroy(&cfg);

    printf("[MAIN] Service execution cleanup sequence finalized successfully. Done.\n");
    return EXIT_SUCCESS;
}
/*
 * src/work/pcap_producer.c
 * Packet Capture Producer Work Implementation (ANSI C)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "workmgr/workmgr.h"
#include "algorithms/queue/queue.h"
#include <libconfig.h>

/* Local configuration frame wrapper for internal pcap descriptors */
typedef struct {
    pcap_t *pcap_handle;
    const char *interface_name;
    char *capture_filter;
    size_t max_payload_bytes;
    double sampling_rate;
} PcapProducerState_t;

static void i_pcap_producer_setup(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    char errbuf[PCAP_ERRBUF_SIZE];
    if (!ctx || !ctx->custom_config) {
        fprintf(stderr, "[PCAP_PRODUCER][FATAL] Work configuration section missing.\n");
        exit(EXIT_FAILURE);
    }

    /* Expect custom_config to be config_setting_t* pointing to works.pcap_producer */
    config_setting_t *setting = (config_setting_t *)ctx->custom_config;

    PcapProducerState_t *state = (PcapProducerState_t *)malloc(sizeof(PcapProducerState_t));
    if (!state) {
        fprintf(stderr, "[PCAP_PRODUCER][FATAL] Memory allocation failed for local state structure.\n");
        exit(EXIT_FAILURE);
    }
    /* Read first interface from config list */
    config_setting_t *iflist = config_setting_lookup(setting, "interfaces");
    const char *if_name = NULL;
    if (iflist && config_setting_length(iflist) > 0) {
        if_name = config_setting_get_string_elem(iflist, 0);
    }

    if (!if_name) {
        fprintf(stderr, "[PCAP_PRODUCER][FATAL] No interface configured in works.pcap_producer.interfaces.\n");
        free(state);
        exit(EXIT_FAILURE);
    }

    state->interface_name = if_name;
    /* read optional configuration values */
    const char *filter_str = NULL;
    if (config_setting_lookup_string(setting, "capture_filter", &filter_str) == CONFIG_TRUE && filter_str != NULL) {
        state->capture_filter = strdup(filter_str);
    } else {
        state->capture_filter = NULL;
    }

    int max_payload = 0;
    if (config_setting_lookup_int(setting, "max_payload_bytes", &max_payload) == CONFIG_TRUE && max_payload > 0) {
        state->max_payload_bytes = (size_t)max_payload;
    } else {
        state->max_payload_bytes = 65535; /* default: full packet */
    }

    double sampling = 1.0;
    if (config_setting_lookup_float(setting, "sampling_rate", &sampling) == CONFIG_TRUE) {
        if (sampling <= 0.0) sampling = 0.0;
        if (sampling > 1.0) sampling = 1.0;
    }
    state->sampling_rate = sampling;
    
    /* Open the interface in promiscuous mode with a standard 65535 byte snaplen limit */
    state->pcap_handle = pcap_open_live(state->interface_name, 65535, 1, 1000, errbuf);
    if (!state->pcap_handle) {
        fprintf(stderr, "[PCAP_PRODUCER][ERROR] Could not activate interface '%s': %s\n", state->interface_name, errbuf);
        free(state);
        exit(EXIT_FAILURE);
    }

    /* If a capture filter is configured, compile and apply it. Non-fatal on failure. */
    if (state->capture_filter) {
        struct bpf_program fp;
        if (pcap_compile(state->pcap_handle, &fp, state->capture_filter, 1, PCAP_NETMASK_UNKNOWN) == 0) {
            if (pcap_setfilter(state->pcap_handle, &fp) != 0) {
                fprintf(stderr, "[PCAP_PRODUCER][WARN] Failed to set BPF filter '%s' on %s\n", state->capture_filter, state->interface_name);
            }
            pcap_freecode(&fp);
        } else {
            fprintf(stderr, "[PCAP_PRODUCER][WARN] Failed to compile BPF filter '%s'\n", state->capture_filter);
        }
    }

    /* Seed PRNG for sampling decisions per-producer process */
    srand((unsigned int)(time(NULL) ^ getpid()));

    /* Override custom_config pointer to retain our dynamic local state inside this fork */
    ctx->custom_config = (void *)state;
    
    /* Safely increment active tracking counter inside the process-shared queue */
    queue_producer_new(ctx->pipeline);
    printf("[PCAP_PRODUCER] Capture interface stream '%s' successfully attached.\n", state->interface_name);
}

static void i_pcap_producer_run_loop(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    PcapProducerState_t *state = (PcapProducerState_t *)ctx->custom_config;
    struct pcap_pkthdr header;
    const u_char *packet;

    while (1) {
        packet = pcap_next(state->pcap_handle, &header);
        if (packet == NULL) continue;

        /* Probabilistic sampling */
        if (state->sampling_rate < 1.0) {
            double r = (double)rand() / (double)RAND_MAX;
            if (r > state->sampling_rate) {
                continue; /* drop packet due to sampling */
            }
        }

        /* Apply payload truncation if configured */
        size_t write_len = header.caplen;
        if (write_len > state->max_payload_bytes) write_len = state->max_payload_bytes;

        int write_res = queue_write(ctx->pipeline, packet, write_len, NULL);
        if (write_res == QUEUE_RET_DESTROYING) {
            printf("[PCAP_PRODUCER] Shared pipeline is being torn down. Terminating worker execution frame.\n");
            break;
        }
    }
}

static void i_pcap_producer_teardown(void *raw_ctx) {
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    if (!ctx || !ctx->custom_config) return;

    PcapProducerState_t *state = (PcapProducerState_t *)ctx->custom_config;
    
    if (state->pcap_handle) {
        pcap_close(state->pcap_handle);
    }
    
    printf("[PCAP_PRODUCER] Capture session closed cleanly for device interface '%s'.\n", state->interface_name);
    queue_producer_free(ctx->pipeline);
    if (state->capture_filter) free(state->capture_filter);
    free(state);
}

/* Public global registration descriptor reference used inside main.c */
WorkDescriptor_t pcap_producer_work = {
    .work_type_name = "PCAP_PRODUCER",
    .config_name = "pcap_producer",
    .work_setup = i_pcap_producer_setup,
    .work_run_loop = i_pcap_producer_run_loop,
    .work_teardown = i_pcap_producer_teardown
};
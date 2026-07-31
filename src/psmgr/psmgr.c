/*
 * psmgr.c
 * Process Supervisor Manager Implementation (ANSI C / POSIX)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#include "psmgr.h"

typedef struct {
    pid_t pid;
    char name[64];
    bool is_active;
} TrackedProcess_t;

static TrackedProcess_t *g_process_table = NULL;
static int g_max_slots = 0;
static int g_active_count = 0;

int psmgr_init(int max_process_slots) {
    if (max_process_slots <= 0) return PSMGR_ERROR;
    
    g_process_table = (TrackedProcess_t*)calloc(max_process_slots, sizeof(TrackedProcess_t));
    if (!g_process_table) {
        fprintf(stderr, "[PSMGR][FATAL] Failed to allocate memory for process tracking tracking tables.\n");
        return PSMGR_ERROR;
    }
    
    g_max_slots = max_process_slots;
    g_active_count = 0;
    return PSMGR_SUCCESS;
}

pid_t psmgr_spawn_worker(const char *name, psmgr_worker_fn entry_point, void *ctx) {
    int target_slot = -1;
    int i;
    pid_t pid;

    if (!entry_point || !name) return PSMGR_ERROR;

    /* Find an available slot in our execution monitoring registry */
    for (i = 0; i < g_max_slots; i++) {
        if (!g_process_table[i].is_active) {
            target_slot = i;
            break;
        }
    }

    if (target_slot == -1) {
        fprintf(stderr, "[PSMGR][ERROR] Process tracking table limit exhausted (%d slots).\n", g_max_slots);
        return PSMGR_ERR_FULL;
    }

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "[PSMGR][ERROR] Sub-process instantiation failed via fork(): %s\n", strerror(errno));
        return PSMGR_ERROR;
    }

    if (pid == 0) {
        /* ------------------------------------------------------------ */
        /* CHILD PROCESS EXECUTION FRAME                                */
        /* ------------------------------------------------------------ */
        
        /* Reset signal masking properties back to default configurations inside the fork */
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGINT,  &sa, NULL);

        printf("[PSMGR][CHILD] Sub-process '%s' (PID: %d) entering target functional execution frame.\n", name, getpid());
        
        /* Execute the modular assigned workload loop */
        entry_point(ctx);
        
        printf("[PSMGR][CHILD] Sub-process '%s' (PID: %d) exited execution loop cleanly.\n", name, getpid());
        exit(EXIT_SUCCESS);
    }

    /* ------------------------------------------------------------ */
    /* MASTER PROCESS EXECUTION FRAME                               */
    /* ------------------------------------------------------------ */
    g_process_table[target_slot].pid = pid;
    g_process_table[target_slot].is_active = true;
    strncpy(g_process_table[target_slot].name, name, sizeof(g_process_table[target_slot].name) - 1);
    g_process_table[target_slot].name[sizeof(g_process_table[target_slot].name) - 1] = '\0';
    
    g_active_count++;
    printf("[PSMGR][MASTER] Successfully registered worker process '%s' under PID %d at registry slot %d.\n", name, pid, target_slot);
    
    return pid;
}

void psmgr_periodic_check(void) {
    int i;
    for (i = 0; i < g_max_slots; i++) {
        if (!g_process_table[i].is_active) continue;

        int status;
        /* Non-blocking harvest invocation query */
        pid_t res = waitpid(g_process_table[i].pid, &status, WNOHANG);

        if (res > 0) {
            /* Child changed layout state or died */
            g_process_table[i].is_active = false;
            g_active_count--;

            if (WIFEXITED(status)) {
                printf("[PSMGR][INFO] Worker '%s' (PID: %d) terminated normally with code %d.\n",
                       g_process_table[i].name, res, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "[PSMGR][WARN] Worker '%s' (PID: %d) crashed via unhandled signal %d (%s)!\n",
                        g_process_table[i].name, res, WTERMSIG(status), strsignal(WTERMSIG(status)));
            }
        } else if (res == -1) {
            if (errno != ECHILD) {
                fprintf(stderr, "[PSMGR][ERROR] waitpid failure inspecting worker index %d: %s\n", i, strerror(errno));
            } else {
                /* System reaped the child elsewhere, clear slots safely */
                g_process_table[i].is_active = false;
                g_active_count--;
            }
        }
    }
}

void psmgr_stop_all_workers(void) {
    int i, loop_limit;
    int remaining_workers;

    if (!g_process_table || g_active_count == 0) return;

    printf("[PSMGR] Commencing graceful shutdown sequence for %d active subprocesses...\n", g_active_count);

    /* Phase 1: Issue SIGTERM softly across all active targets */
    for (i = 0; i < g_max_slots; i++) {
        if (g_process_table[i].is_active) {
            printf("[PSMGR] Transmitting SIGTERM to worker '%s' (PID: %d)...\n", g_process_table[i].name, g_process_table[i].pid);
            kill(g_process_table[i].pid, SIGTERM);
        }
    }

    /* Phase 2: Give workers up to 3 seconds total to handle internal pipelines and exit */
    remaining_workers = g_active_count;
    for (loop_limit = 0; loop_limit < 30 && remaining_workers > 0; loop_limit++) {
        usleep(100000); /* Intercept changes every 100 milliseconds */
        
        for (i = 0; i < g_max_slots; i++) {
            if (!g_process_table[i].is_active) continue;

            int status;
            pid_t res = waitpid(g_process_table[i].pid, &status, WNOHANG);
            if (res > 0 || (res == -1 && errno == ECHILD)) {
                g_process_table[i].is_active = false;
                remaining_workers--;
                g_active_count--;
            }
        }
    }

    /* Phase 3: Forcefully escalate remaining unresponsive targets to SIGKILL */
    for (i = 0; i < g_max_slots; i++) {
        if (g_process_table[i].is_active) {
            fprintf(stderr, "[PSMGR][WARN] Worker '%s' (PID: %d) ignored SIGTERM. Escalating to SIGKILL.\n", 
                    g_process_table[i].name, g_process_table[i].pid);
            kill(g_process_table[i].pid, SIGKILL);
            
            /* Clean up zombie processes instantly since SIGKILL cannot be blocked or handled */
            waitpid(g_process_table[i].pid, NULL, 0);
            g_process_table[i].is_active = false;
            g_active_count--;
        }
    }

    printf("[PSMGR] All registered sub-processes cleared successfully.\n");
}

void psmgr_destroy(void) {
    if (g_process_table) {
        free(g_process_table);
        g_process_table = NULL;
    }
    g_max_slots = 0;
    g_active_count = 0;
}
#include "psmgr.h"
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include "algorithms/telemetry/telemetry.h"
#include "algorithms/dlist/dlist.h"

/* Local constant definitions replacing external global kernel status mappings */
#define PSMGR_STATUS_RUNNING 1
#define PSMGR_STATUS_STOPPED 0

typedef struct {
    pid_t pid;
    psmgr_proc_type_t proc_type;
    char short_name[16];
    const char *full_name;
    psmgr_exec_routine_t exec_fn;
    void *arg;
} psmgr_process_t;

typedef struct {
    dlist_t *proc_list;
    volatile sig_atomic_t system_status;
} psmgr_config_t;

static psmgr_config_t g_psmgr_cfg;

static void i_psmgr_handle_terminate(int signo) {
    (void)signo;
    /* Isolated local assignment bypassing global kernel dependency loops */
    g_psmgr_cfg.system_status = PSMGR_STATUS_STOPPED;
}

static void i_psmgr_handle_child_crash(int signo) {
    (void)signo;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFSIGNALED(status)) {
            printf("[PSMGR][CRIT] Isolated Child Process [PID: %d] CRASHED! Killed by signal: %d\n", 
                   pid, WTERMSIG(status));
        } else if (WIFEXITED(status)) {
            printf("[PSMGR][INFO] Child Process [PID: %d] exited. Return code: %d\n", 
                   pid, WEXITSTATUS(status));
        }
    }
}

typedef struct {
    int signo;
    struct sigaction sigcfg;
} signal_mapping_t;

/* FIXED: Corrected initialization syntax structure to remove implicit field mismatch errors */
static const signal_mapping_t g_signals_new[] = {
    {SIGCHLD, {.sa_handler = i_psmgr_handle_child_crash, .sa_flags = SA_RESTART | SA_NOCLDSTOP}},
    {SIGTERM, {.sa_handler = i_psmgr_handle_terminate,   .sa_flags = SA_SIGINFO}},
    {SIGINT,  {.sa_handler = i_psmgr_handle_terminate,   .sa_flags = SA_SIGINFO}}
};

static signal_mapping_t g_signals_old[] = {
    {SIGCHLD, {{0}}},
    {SIGTERM, {{0}}},
    {SIGINT,  {{0}}}
};

// ====================================================================
// EXTERNAL API: PUBLIC RUNTIME SCHEDULING INTERFACES
// ====================================================================

int psmgr_start(void) {
    g_psmgr_cfg.system_status = PSMGR_STATUS_RUNNING;
    
    int elements = sizeof(g_signals_new) / sizeof(g_signals_new[0]);
    for (int i = 0; i < elements; i++) {
        if (sigaction(g_signals_new[i].signo, &g_signals_new[i].sigcfg, &g_signals_old[i].sigcfg) != 0) {
            return -1;
        }
    }

    g_psmgr_cfg.proc_list = (dlist_t*)dlist_alloc();
    return (g_psmgr_cfg.proc_list == NULL) ? -1 : 0;
}

void psmgr_stop(void) {
    g_psmgr_cfg.system_status = PSMGR_STATUS_STOPPED;
    int elements = sizeof(g_signals_old) / sizeof(g_signals_old[0]);
    for (int i = 0; i < elements; i++) {
        sigaction(g_signals_old[i].signo, &g_signals_old[i].sigcfg, NULL);
    }

    if (g_psmgr_cfg.proc_list != NULL) {
        dlist_free_deleted_entries(g_psmgr_cfg.proc_list);
        dlist_free(g_psmgr_cfg.proc_list);
        g_psmgr_cfg.proc_list = NULL;
    }
}

int psmgr_idle(uint64_t wait_time_in_sec) {
    if (g_psmgr_cfg.proc_list && !dlist_is_empty(g_psmgr_cfg.proc_list)) {
        sleep((unsigned int)wait_time_in_sec);
    }
    if (g_psmgr_cfg.proc_list) {
        dlist_free_deleted_entries(g_psmgr_cfg.proc_list);
    }
    return 0;
}

void psmgr_stop_user_processes(void) {
    if (g_psmgr_cfg.proc_list == NULL) return;
    while (!dlist_is_empty(g_psmgr_cfg.proc_list)) {
        sleep(1);
    }
}

int psmgr_create_process(const char *short_name, const char *full_name, psmgr_proc_type_t type, psmgr_exec_routine_t exec_fn, void *arg, pid_t *out_pid) {
    if (short_name == NULL || full_name == NULL || exec_fn == NULL)
        return -EINVAL;

    if (strlen(short_name) >= 16)
        return -EINVAL;

    pid_t pid = fork();
    if (pid < 0) return -ENOMEM;

    if (pid == 0) {
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        pthread_setname_np(pthread_self(), short_name);
        
        int child_ret = exec_fn(arg);
        exit(child_ret);
    }

    psmgr_process_t *proc_node = (psmgr_process_t*)malloc(sizeof(psmgr_process_t));
    if (proc_node != NULL) {
        proc_node->pid = pid;
        proc_node->proc_type = type;
        proc_node->full_name = full_name;
        strncpy(proc_node->short_name, short_name, sizeof(proc_node->short_name) - 1);
        proc_node->short_name[sizeof(proc_node->short_name) - 1] = '\0';
        
        dlist_add(g_psmgr_cfg.proc_list, (uint64_t)pid, proc_node, sizeof(psmgr_process_t));
        dlist_release(proc_node);
    }

    if (out_pid != NULL) *out_pid = pid;
    return 0;
}

int psmgr_wait_for_process(pid_t pid, int *out_exit_status) {
    int status;
    pid_t r = waitpid(pid, &status, 0);
    if (r < 0) return -1;

    if (WIFEXITED(status)) {
        if (out_exit_status) *out_exit_status = WEXITSTATUS(status);
        return 0;
    }
    if (WIFSIGNALED(status)) {
        if (out_exit_status) *out_exit_status = -WTERMSIG(status);
        return 0;
    }
    return -1;
}
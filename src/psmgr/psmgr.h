#ifndef _PS_MANAGER_H
#define _PS_MANAGER_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

/* Functional callback prototypes for process execution loops - returning standard int status */
typedef int (*psmgr_exec_routine_t)(void *);

/* Process classification categories mapped to system privileges */
typedef enum {
    PSMGR_PROC_KERNEL = 1,
    PSMGR_PROC_USER   = 2
} psmgr_proc_type_t;

/* Public API Management Lifecycles (Returns 0 on success, negative on failure) */
int psmgr_start(void);
void psmgr_stop(void);
void psmgr_stop_user_processes(void);
int psmgr_idle(uint64_t wait_time_in_sec);

/* Public API Core Process Creation and Isolation Hooks */
int psmgr_create_process(
    const char *short_name, 
    const char *full_name, 
    psmgr_proc_type_t type, 
    psmgr_exec_routine_t exec_fn, 
    void *arg,
    pid_t *out_pid
);

int psmgr_wait_for_process(pid_t pid, int *out_exit_status);

#endif /* _PS_MANAGER_H */
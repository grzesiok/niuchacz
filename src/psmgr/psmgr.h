/*
 * psmgr.h
 * Process Supervisor Manager Interface (ANSI C)
 * Handles isolated process spawning, fault detection, and graceful teardown.
 */

#ifndef PSMGR_H
#define PSMGR_H

#include <sys/types.h>
#include <stdbool.h>

#define PSMGR_SUCCESS         0
#define PSMGR_ERROR          -1
#define PSMGR_ERR_FULL       -2

/* 
 * Callback signature for worker functions executed inside the child process.
 * Acceptable context includes references to shared queues, identifiers, or database parameters.
 */
typedef void (*psmgr_worker_fn)(void *ctx);

/* Initializes the internal process tracking supervisor structures */
int psmgr_init(int max_process_slots);

/* 
 * Spawns an isolated sub-process via fork() execution frames.
 * Captures failures locally without terminating the main daemon framework.
 */
pid_t psmgr_spawn_worker(const char *name, psmgr_worker_fn entry_point, void *ctx);

/* 
 * Non-blocking periodic check sweep loop.
 * Reaps exited/zombie child processes, tracks statuses, and identifies crashes.
 */
void psmgr_periodic_check(void);

/* 
 * Shuts down all active registered child processes cleanly.
 * Uses a progressive timed signals escalation cycle (SIGTERM -> SIGKILL).
 */
void psmgr_stop_all_workers(void);

/* Free internal arrays and resource footprints managed by the supervisor */
void psmgr_destroy(void);

#endif /* PSMGR_H */
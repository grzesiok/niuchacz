#ifndef _LIBALGORITHMS_ALGORITHMS_TIMER_H
#define _LIBALGORITHMS_ALGORITHMS_TIMER_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/* Public API Conversion and Evaluation Routines */
uint64_t timerTimespecToNs(const struct timespec *ts);
int timerCmp(const struct timespec *ts1, const struct timespec *ts2);
bool timerIsNull(const struct timespec *ts);

/* Public API Timestamp Generation Privileges */
void timerGetRealCurrentTimestamp(struct timespec *ts);
void timerGetThreadCurrentTimestamp(struct timespec *ts);
void timerGetMonotonicCurrentTimestamp(struct timespec *ts);

/* Public API Performance Measurement Stopwatch Hooks */
void timerWatchStart(struct timespec *ts);
uint64_t timerWatchStep(struct timespec *ts);
uint64_t timerWatchStop(const struct timespec *startTime);

#endif /* _LIBALGORITHMS_ALGORITHMS_TIMER_H */
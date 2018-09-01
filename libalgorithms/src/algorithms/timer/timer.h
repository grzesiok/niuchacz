#ifndef _LIBALGORITHMS_ALGORITHMS_TIMER_H
#define _LIBALGORITHMS_ALGORITHMS_TIMER_H
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

uint64_t timerTimespecToNs(struct timespec *ts);
void timerGetRealCurrentTimestamp(struct timespec *ts);
void timerGetThreadCurrentTimestamp(struct timespec *ts);
int timerCmp(struct timespec *ts1, struct timespec *ts2);
bool timerIsNull(struct timespec *ts);
void timerWatchStart(struct timespec *ts);
uint64_t timerWatchStop(struct timespec startTime);
#endif /*_LIBALGORITHMS_ALGORITHMS_TIMER_H */

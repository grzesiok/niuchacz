#include "timer.h"

uint64_t timerTimespecToNs(const struct timespec *ts) {
    if (ts == NULL)
        return 0;
    /* Explicit ULL literal promotion enforces safe 64-bit integer multiplication bounds */
    return ((uint64_t)ts->tv_sec * 1000000000ULL) + (uint64_t)ts->tv_nsec;
}

void timerGetRealCurrentTimestamp(struct timespec *ts) {
    clock_gettime(CLOCK_REALTIME, ts);
}

void timerGetThreadCurrentTimestamp(struct timespec *ts) {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
}

void timerGetMonotonicCurrentTimestamp(struct timespec *ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

int timerCmp(const struct timespec *ts1, const struct timespec *ts2) {
    uint64_t ns1 = timerTimespecToNs(ts1);
    uint64_t ns2 = timerTimespecToNs(ts2);
    
    if (ns1 < ns2) return -1;
    if (ns1 > ns2) return 1;
    return 0;
}

bool timerIsNull(const struct timespec *ts) {
    return (ts == NULL || (ts->tv_sec == 0 && ts->tv_nsec == 0));
}

void timerWatchStart(struct timespec *ts) {
    /* Upgraded to CLOCK_MONOTONIC to measure true elapsed wall-clock latency profiles */
    timerGetMonotonicCurrentTimestamp(ts);
}

uint64_t timerWatchStep(struct timespec *ts) {
    if (ts == NULL)
        return 0;
    uint64_t stepTime = timerWatchStop(ts);
    timerWatchStart(ts);
    return stepTime;
}

uint64_t timerWatchStop(const struct timespec *startTime) {
    if (startTime == NULL)
        return 0;

    struct timespec stopTime;
    timerGetMonotonicCurrentTimestamp(&stopTime);
    
    uint64_t start_ns = timerTimespecToNs(startTime);
    uint64_t stop_ns = timerTimespecToNs(&stopTime);
    
    /* Handle potential edge cases where stop timestamp is captured before start bounds */
    if (stop_ns <= start_ns)
        return 0;
        
    return (stop_ns - start_ns);
}
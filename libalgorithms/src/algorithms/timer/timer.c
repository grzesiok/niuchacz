#include "timer.h"
#include <sys/time.h>

uint64_t timerTimespecToNs(struct timespec *ts) {
    return ts->tv_sec*1000000000 + ts->tv_nsec;
}

void timerGetRealCurrentTimestamp(struct timespec *ts) {
    clock_gettime(CLOCK_REALTIME, ts);
}

void timerGetThreadCurrentTimestamp(struct timespec *ts) {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
}

int timerCmp(struct timespec *ts1, struct timespec *ts2) {
    uint64_t ullpts1 = timerTimespecToNs(ts1);
    uint64_t ullpts2 = timerTimespecToNs(ts2);
    if(ullpts1 == ullpts2)
        return 0;
    if(ullpts1 < ullpts2)
        return -1;
    return 1;
}

bool timerIsNull(struct timespec *ts) {
    return (ts == NULL || (ts->tv_sec == 0 && ts->tv_nsec == 0));
}

void timerWatchStart(struct timespec *ts) {
    timerGetThreadCurrentTimestamp(ts);
}

uint64_t timerWatchStep(struct timespec *ts) {
    uint64_t stepTime = timerWatchStop(*ts);
    timerWatchStart(ts);
    return stepTime;
}

uint64_t timerWatchStop(struct timespec startTime) {
    struct timespec stopTime;
    timerGetThreadCurrentTimestamp(&stopTime);
    struct timespec resultTime;
    if((stopTime.tv_nsec - startTime.tv_nsec) < 0) {
    	resultTime.tv_sec = stopTime.tv_sec - startTime.tv_sec - 1;
    	resultTime.tv_nsec = stopTime.tv_nsec - startTime.tv_nsec + 1000000000;
    } else {
    	resultTime.tv_sec = stopTime.tv_sec - startTime.tv_sec;
    	resultTime.tv_nsec = stopTime.tv_nsec - startTime.tv_nsec;
    }
    return timerTimespecToNs(&resultTime);
}

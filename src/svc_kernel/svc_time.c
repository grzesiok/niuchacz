#include "svc_time.h"

struct timespec timerStart(void) {
    struct timespec startTime;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);
    return startTime;
}

unsigned long long timerStop(struct timespec startTime) {
    struct timespec stopTime;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stopTime);
    unsigned long long diffInNanos = (unsigned long long)stopTime.tv_nsec - (unsigned long long)startTime.tv_nsec;
    return diffInNanos;
}

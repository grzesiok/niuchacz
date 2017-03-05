#include "svc_time.h"

struct timespec timerStart(void) {
    struct timespec startTime;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);
    return startTime;
}

unsigned long long timerStop(struct timespec startTime) {
    struct timespec stopTime;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stopTime);
    struct timespec resultTime;
    if((stopTime.tv_nsec - startTime.tv_nsec) < 0) {
    	resultTime.tv_sec = stopTime.tv_sec - startTime.tv_sec - 1;
    	resultTime.tv_nsec = stopTime.tv_nsec - startTime.tv_nsec + 1000000000;
    } else {
    	resultTime.tv_sec = stopTime.tv_sec - startTime.tv_sec;
    	resultTime.tv_nsec = stopTime.tv_nsec - startTime.tv_nsec;
    }
    unsigned long long diffInNanos = resultTime.tv_sec*1000000000 + resultTime.tv_nsec;
    return diffInNanos;
}

#ifndef _SVC_TIME_H
#define _SVC_TIME_H
#include <time.h>

unsigned long long timerCurrentTimestamp(void);
struct timespec timerStart(void);
unsigned long long timerStop(struct timespec startTime);
#endif

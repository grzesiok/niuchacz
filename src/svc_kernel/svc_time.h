#ifndef _SVC_TIME_H
#define _SVC_TIME_H
#include <time.h>

struct timespec timerStart(void);
unsigned long long timerStop(struct timespec startTime);
#endif

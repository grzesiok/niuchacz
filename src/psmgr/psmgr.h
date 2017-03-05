#ifndef _PS_MANAGER_H
#define _PS_MANAGER_H
#include "../svc_kernel/svc_kernel.h"
#include <pthread.h>

KSTATUS psmgrStart(void);
void psmgrStop(void);
void psmgrIdle(unsigned long long waitTimeInSec);
KSTATUS psmgrCreateThread(void *(*p_startRoutine) (void *), void *p_arg);
#endif

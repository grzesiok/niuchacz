#ifndef _PS_MANAGER_H
#define _PS_MANAGER_H
#include "svc_kernel/svc_kernel.h"
#include <pthread.h>

typedef KSTATUS (*psmgr_execRoutine)(void *);
typedef void (*psmgr_cancelRoutine)(void *);

KSTATUS psmgrStart(void);
void psmgrStop(void);
void psmgrStopUserThreads(void);
KSTATUS psmgrIdle(unsigned long long waitTimeInSec);

#define PSMGR_THREAD_KERNEL 1
#define PSMGR_THREAD_USER 2
KSTATUS psmgrCreateThread(const char* c_shortThreadName, const char* c_fullThreadName, int threadType, psmgr_execRoutine p_execRoutine, psmgr_cancelRoutine p_cancelRoutine, void *p_arg);
KSTATUS psmgrWaitForThread(pthread_t threadId);
#endif

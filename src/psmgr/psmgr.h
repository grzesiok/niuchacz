#ifndef _PS_MANAGER_H
#define _PS_MANAGER_H
#include "../svc_kernel/svc_kernel.h"
#include <pthread.h>

typedef KSTATUS (*psmgr_execRoutine)(void *);
typedef void (*psmgr_cancelRoutine)(void *);

KSTATUS psmgrStart(void);
void psmgrStop(void);
KSTATUS psmgrCreateThread(const char* c_threadName, psmgr_execRoutine p_execRoutine, psmgr_cancelRoutine p_cancelRoutine, void *p_arg);
#endif

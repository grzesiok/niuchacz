#ifndef _PS_MANAGER_H
#define _PS_MANAGER_H
#include "../svc_kernel/svc_kernel.h"
#include <pthread.h>

KSTATUS psmgr_start(void);
void psmgr_stop(void);
void psmgr_idle(void);
KSTATUS psmgr_create_thread(void *(*start_routine) (void *), void *arg);
#endif

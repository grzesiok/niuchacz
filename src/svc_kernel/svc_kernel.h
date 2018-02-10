#ifndef _SVC_KERNEL_H
#define _SVC_KERNEL_H
#include "../kernel.h"
#include "svc_status.h"
#include "svc_lock.h"
#include "svc_statistics.h"
#include "svc_time.h"
#include "../psmgr/psmgr.h"
#include "execute_unit/cmd_manager.h"
#include "database/database.h"

#define SVC_KERNEL_STATUS_START_PENDING 1
#define SVC_KERNEL_STATUS_RUNNING 2
#define SVC_KERNEL_STATUS_STOP_PENDING 3

KSTATUS svcKernelInit(void);
void svcKernelExit(int code) NORETURN;
KSTATUS svcKernelStatus(int requested_status);
int svcKernelGetCurrentStatus(void);
sqlite3* svcKernelGetDb(void);
#define svcKernelIsRunning() (svcKernelGetCurrentStatus() == SVC_KERNEL_STATUS_RUNNING)
#endif

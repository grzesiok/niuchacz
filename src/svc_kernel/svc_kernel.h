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
#include <libconfig.h>

typedef struct _KERNEL
{
	volatile int _status;
	sqlite3* _db;
	config_t _cfg;
} KERNEL, *PKERNEL;

#define SVC_KERNEL_STATUS_START_PENDING 1
#define SVC_KERNEL_STATUS_RUNNING 2
#define SVC_KERNEL_STATUS_STOP_PENDING 3

KSTATUS svcKernelInit(const char* confFileName);
void svcKernelExit(int code) NORETURN;
KSTATUS svcKernelStatus(int requested_status);
int svcKernelGetCurrentStatus(void);
sqlite3* svcKernelGetDb(void);
PKERNEL svcKernelGetCfg(void);
#define svcKernelIsRunning() (svcKernelGetCurrentStatus() == SVC_KERNEL_STATUS_RUNNING)
#endif

#ifndef _SVC_KERNEL_H
#define _SVC_KERNEL_H
#include "kernel.h"
#include "svc_kernel/svc_status.h"
#include "svc_kernel/svc_statistics.h"
#include "svc_kernel/database/database.h"
#include "svc_kernel/svc_lock.h"
#include "svc_kernel/svc_update.h"
#include "svc_kernel/psmgr/psmgr.h"
#include "svc_kernel/execute_unit/cmd_manager.h"
#include <libconfig.h>

typedef struct _KERNEL
{
    volatile int _status;
    database_t* _db;
    config_t _cfg;
    stats_list_t* _stats_list;
} KERNEL, *PKERNEL;

#define SVC_KERNEL_STATUS_START_PENDING 1
#define SVC_KERNEL_STATUS_RUNNING 2
#define SVC_KERNEL_STATUS_STOP_PENDING 3

KSTATUS svcKernelInit(const char* confFileName);
void svcKernelExit(int code) NORETURN;
void svcKernelMainLoop(void);
KSTATUS svcKernelStatus(int requested_status);
int svcKernelGetCurrentStatus(void);
database_t* svcKernelGetDb(void);
config_t* svcKernelGetCfg(void);
#define svcKernelIsRunning() (svcKernelGetCurrentStatus() == SVC_KERNEL_STATUS_RUNNING || svcKernelGetCurrentStatus() == SVC_KERNEL_STATUS_START_PENDING)
#endif

#ifndef _SVC_KERNEL_H
#define _SVC_KERNEL_H
#include "../kernel.h"
#include "../svc_kernel/svc_status.h"
#include "../svc_kernel/svc_lock.h"
#include "../svc_kernel/svc_log.h"

#define SVC_KERNEL_STATUS_START_PENDING 1
#define SVC_KERNEL_STATUS_RUNNING 2
#define SVC_KERNEL_STATUS_STOP_PENDING 3
#define SVC_KERNEL_STATUS_STOPPED 4

KSTATUS svc_kernel_status(int requested_status);
int svc_kernel_get_current_status(void);
#define svc_kernel_is_running() (svc_kernel_get_current_status() == SVC_KERNEL_STATUS_RUNNING)
#endif
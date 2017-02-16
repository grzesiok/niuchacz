#include "svc_kernel.h"

typedef struct _KERNEL
{
	volatile int _status;
} KERNEL, *PKERNEL;

static KERNEL gKernelCfg;

KSTATUS svc_kernel_init(void)
{
	__atomic_store_n(&gKernelCfg._status, SVC_KERNEL_STATUS_START_PENDING, __ATOMIC_RELEASE);
	return KSTATUS_SUCCESS;
}

KSTATUS svc_kernel_status(int requested_status)
{
	int old_status = __atomic_load_n(&gKernelCfg._status, __ATOMIC_ACQUIRE);
	if(requested_status == SVC_KERNEL_STATUS_STOP_PENDING
			&& (old_status == SVC_KERNEL_STATUS_START_PENDING || old_status == SVC_KERNEL_STATUS_RUNNING))
	{
		__atomic_store_n(&gKernelCfg._status, SVC_KERNEL_STATUS_STOP_PENDING, __ATOMIC_RELEASE);
	} else if(requested_status == SVC_KERNEL_STATUS_RUNNING
			&& old_status == SVC_KERNEL_STATUS_START_PENDING)
	{
		__atomic_store_n(&gKernelCfg._status, SVC_KERNEL_STATUS_RUNNING, __ATOMIC_RELEASE);
	}
}

int svc_kernel_get_current_status(void)
{
	return __atomic_load_n(&gKernelCfg._status, __ATOMIC_ACQUIRE);
}

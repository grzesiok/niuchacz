#include "svc_kernel.h"

typedef struct _KERNEL
{
	volatile int _status;
} KERNEL, *PKERNEL;

static KERNEL gKernelCfg;

KSTATUS svc_kernel_status(int requested_status)
{
	gKernelCfg._status = requested_status;
}

int svc_kernel_get_current_status(void)
{
	return gKernelCfg._status;
}

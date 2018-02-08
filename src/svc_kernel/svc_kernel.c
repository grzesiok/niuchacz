#include "svc_kernel.h"
#include <signal.h>

typedef struct _KERNEL
{
	volatile int _status;
} KERNEL, *PKERNEL;

static KERNEL gKernelCfg;

void svc_kernel_sig_handler(int signo)
{
	if(signo == SIGINT)
		svc_kernel_status(SVC_KERNEL_STATUS_STOP_PENDING);
}

KSTATUS svc_kernel_init(void)
{
	KSTATUS _status;

	/* Open system log and write message to it */
	openlog("NIUCHACZ", LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Starting...");

	__atomic_store_n(&gKernelCfg._status, SVC_KERNEL_STATUS_START_PENDING, __ATOMIC_RELEASE);
	if(signal(SIGINT, svc_kernel_sig_handler) == SIG_ERR)
		return KSTATUS_UNSUCCESS;
	_status = statsStart();
	if(!KSUCCESS(_status))
		return _status;
	_status = psmgrStart();
	if(!KSUCCESS(_status))
		return _status;
	_status = cmdmgrStart();
	if(!KSUCCESS(_status))
		return _status;

	syslog(LOG_INFO, "Started");
	return KSTATUS_SUCCESS;
}

void svc_kernel_exit(int code)
{
	syslog(LOG_INFO, "Stopping...");
	cmdmgrStop();
	psmgrStop();
	statsStop();
	signal(SIGINT, SIG_DFL);

	/* Write system log and close it. */
	syslog(LOG_INFO, "Stopped");
	closelog();

	exit(code);
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
	} else return KSTATUS_UNSUCCESS;
	return KSTATUS_SUCCESS;
}

int svc_kernel_get_current_status(void)
{
	return __atomic_load_n(&gKernelCfg._status, __ATOMIC_ACQUIRE);
}

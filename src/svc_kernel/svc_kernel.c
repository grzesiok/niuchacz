#include "svc_kernel.h"
#include <signal.h>
#include "database/database.h"

typedef struct _KERNEL
{
	volatile int _status;
	sqlite3* _db;
} KERNEL, *PKERNEL;

static KERNEL gKernelCfg;

void svcKernelSigHandler(int signo) {
	if(signo == SIGINT)
		svcKernelStatus(SVC_KERNEL_STATUS_STOP_PENDING);
}

KSTATUS svcKernelInit(void) {
	KSTATUS _status;

	/* Open system log and write message to it */
	openlog("NIUCHACZ", LOG_PID|LOG_CONS, LOG_DAEMON);
	SYSLOG(LOG_INFO, "Starting...");

	__atomic_store_n(&gKernelCfg._status, SVC_KERNEL_STATUS_START_PENDING, __ATOMIC_RELEASE);
	if(signal(SIGINT, svcKernelSigHandler) == SIG_ERR)
		return KSTATUS_UNSUCCESS;
	_status = statsStart();
	if(!KSUCCESS(_status))
		return _status;
	_status = dbStart(":memory:", &gKernelCfg._db);
	if(!KSUCCESS(_status))
		return _status;
	_status = psmgrStart();
	if(!KSUCCESS(_status))
		return _status;
	_status = cmdmgrStart();
	if(!KSUCCESS(_status))
		return _status;

	SYSLOG(LOG_INFO, "Started");
	return KSTATUS_SUCCESS;
}

void svcKernelExit(int code) {
	SYSLOG(LOG_INFO, "Stopping...");
	cmdmgrStop();
	psmgrStop();
	dbStop(gKernelCfg._db);
	statsStop();
	signal(SIGINT, SIG_DFL);

	/* Write system log and close it. */
	SYSLOG(LOG_INFO, "Stopped");
	closelog();

	exit(code);
}

KSTATUS svcKernelStatus(int requested_status) {
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

int svcKernelGetCurrentStatus(void) {
	return __atomic_load_n(&gKernelCfg._status, __ATOMIC_ACQUIRE);
}

sqlite3* svcKernelGetDb(void) {
	return gKernelCfg._db;
}

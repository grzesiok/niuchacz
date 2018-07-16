#include "svc_kernel.h"
#include <signal.h>
#include "database/database.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

static KERNEL gKernelCfg;

static void svcKernelSigHandler(int signo) {
	mpp_printf("KernelHandler(pid=%d, %d)\n", getpid(), signo);
	if(signo == SIGTERM || signo == SIGINT)
		svcKernelStatus(SVC_KERNEL_STATUS_STOP_PENDING);
	/*if(signo == SIGHUP)
	 * TODO: reload configuration file */
}

#ifndef DEBUG_MODE
static void svcKernelInitService(void) {
	int fd;

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	for(fd = sysconf(_SC_OPEN_MAX);fd > 0;fd--) {
		close(fd);
	}
	/* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");
}
#endif

KSTATUS svcKernelInit(const char* confFileName) {
	KSTATUS _status;

	__atomic_store_n(&gKernelCfg._status, SVC_KERNEL_STATUS_START_PENDING, __ATOMIC_RELEASE);
#ifndef DEBUG_MODE
	svcKernelInitService();
#endif
	/* Open system log and write message to it */
#ifndef DEBUG_MODE
	openlog("NIUCHACZ", LOG_PID|LOG_CONS, LOG_DAEMON);
#else
	openlog("NIUCHACZ_DEBUG", LOG_PID|LOG_CONS, LOG_DAEMON);
#endif
	SYSLOG(LOG_INFO, "Starting...");
	if(signal(SIGINT, svcKernelSigHandler) == SIG_ERR)
		return KSTATUS_UNSUCCESS;
	if(signal(SIGTERM, svcKernelSigHandler) == SIG_ERR)
		return KSTATUS_UNSUCCESS;
	/* Initialize config filesystem */
	config_init(&gKernelCfg._cfg);
	if(!config_read_file(&gKernelCfg._cfg, confFileName)) {
		SYSLOG(LOG_ERR, "%s:%d - %s\n", config_error_file(&gKernelCfg._cfg), config_error_line(&gKernelCfg._cfg), config_error_text(&gKernelCfg._cfg));
		config_destroy(&gKernelCfg._cfg);
		return KSTATUS_UNSUCCESS;
	}
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
	config_destroy(&gKernelCfg._cfg);
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	/* Write system log and close it. */
	SYSLOG(LOG_INFO, "Stopped");
	closelog();

	exit(code);
}

void svcKernelMainLoop(void) {
	while(svcKernelIsRunning()) {
		psmgrIdle(1);
	}
	cmdmgrWaitForAllJobs();
	psmgrStopUserThreads();
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

config_t* svcKernelGetCfg(void) {
	return &gKernelCfg._cfg;
}

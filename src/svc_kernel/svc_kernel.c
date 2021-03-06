#include "svc_kernel/svc_kernel.h"
#include <signal.h>
#include "svc_kernel/database/database.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

static KERNEL gKernelCfg;

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
    SYSLOG(LOG_INFO, "[KERNEL] Starting...");
    /* Initialize config filesystem */
    config_init(&gKernelCfg._cfg);
    if(!config_read_file(&gKernelCfg._cfg, confFileName)) {
        SYSLOG(LOG_ERR, "%s:%d - %s\n", config_error_file(&gKernelCfg._cfg), config_error_line(&gKernelCfg._cfg), config_error_text(&gKernelCfg._cfg));
        config_destroy(&gKernelCfg._cfg);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsmgrStart();
    if(!KSUCCESS(_status))
        return _status;
    gKernelCfg._stats_list = statsCreate("KERNEL");
    if(gKernelCfg._stats_list == NULL)
        return KSTATUS_UNSUCCESS;
    _status = psmgrStart();
    if(!KSUCCESS(_status))
        return _status;
    _status = dbmgrStart();
    if(!KSUCCESS(_status))
        return _status;
    _status = dbOpen("DB_KRNL0", &gKernelCfg._db);
    if(!KSUCCESS(_status))
        return _status;
    _status = cmdmgrStart();
    if(!KSUCCESS(_status))
        return _status;
    _status = svcUpdateStart();
    if(!KSUCCESS(_status))
        return _status;

    SYSLOG(LOG_INFO, "[KERNEL] Started");
    return KSTATUS_SUCCESS;
}

void svcKernelExit(int code) {
    SYSLOG(LOG_INFO, "[KERNEL] Stopping...");
    svcUpdateStop();
    cmdmgrStop();
    dbClose(gKernelCfg._db);
    dbmgrStop();
    psmgrStop();
    statsDestroy(gKernelCfg._stats_list);
    statsmgrStop();
    config_destroy(&gKernelCfg._cfg);

    /* Write system log and close it. */
    SYSLOG(LOG_INFO, "[KERNEL] Stopped");
    closelog();

    exit(code);
}

void svcKernelMainLoop(void) {
    SYSLOG(LOG_INFO, "[KERNEL] Idle Loop Starting...");
    //TODO: Possibility to check stats from sqlite
    int i = 0;
    while(svcKernelIsRunning()) {
        psmgrIdle(1);
        i++;
        if(i > 60) {
            i = 0;
            statsmgrDump();
        }
    }
    SYSLOG(LOG_INFO, "[KERNEL] Idle Loop Stopping...");
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

database_t* svcKernelGetDb(void) {
    return gKernelCfg._db;
}

config_t* svcKernelGetCfg(void) {
    return &gKernelCfg._cfg;
}

stats_list_t* svcKernelGetStatsList(void) {
    return gKernelCfg._stats_list;
}

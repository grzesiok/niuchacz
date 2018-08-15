#include <unistd.h>
#include <stdio.h>
#include "cmd_manager.h"

static const char* cgCreateSchemaCmdList =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, version integer, pexec integer, pcreate integer, pdestroy integer"
		");";
static const char* cgInsertCommand =
		"insert into cmdmgr_cmdlist(code, description, version, pexec, pcreate, pdestroy)"
		"values (?, ?, ?, ?, ?, ?);";

typedef struct {
    queue_t *_pjobQueue;
    unsigned int _activeJobs;
    pthread_t _cmdExecutorThreadId;
} CMD_MANAGER, *PCMD_MANAGER;

CMD_MANAGER gCmdManager;

/* Internal API */

static void* i_cmdmgrFindPtr(const char* cmd, const char* column_name) {
    int rc;
    sqlite3_stmt *stmt;
    char formatted_query[255];
    void* ptr = NULL;

    sprintf(formatted_query, "select %s from cmdmgr_cmdlist where code = ?;", column_name);
    rc = sqlite3_prepare_v2(svcKernelGetDb(), formatted_query, -1, &stmt, 0);
    if(rc != SQLITE_OK) {
    	SYSLOG(LOG_ERR, "Failed to prepare cursor: %s", dbGetErrmsg(svcKernelGetDb()));
        return NULL;
    }
    rc = sqlite3_bind_text(stmt, 1, cmd, -1, SQLITE_STATIC);
    if(rc != SQLITE_OK) {
    	SYSLOG(LOG_ERR, "Failed to bind text(%s): %s", cmd, dbGetErrmsg(svcKernelGetDb()));
        return NULL;
    }

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW) {
    	ptr = (void*)sqlite3_column_int64(stmt, 0);
        DPRINTF("Routine %p(%s) found for cmd=%s", ptr, column_name, cmd);
        return ptr;
    } else SYSLOG(LOG_ERR, "No command found for: %s", cmd);
    rc = sqlite3_finalize(stmt);
    if(rc != SQLITE_OK) {
        SYSLOG(LOG_ERR, "Failed to clear cursor: %s", dbGetErrmsg(svcKernelGetDb()));
        return ptr;
    }
    return ptr;
}

static PJOB_CREATE i_cmdmgrFindCreate(const char* cmd) {
    return (PJOB_CREATE)i_cmdmgrFindPtr(cmd, "pcreate");
}

static PJOB_DESTROY i_cmdmgrFindDestroy(const char* cmd) {
    return (PJOB_DESTROY)i_cmdmgrFindPtr(cmd, "pdestroy");
}

static PJOB_EXEC i_cmdmgrFindExec(const char* cmd) {
    return (PJOB_EXEC)i_cmdmgrFindPtr(cmd, "pexec");
}

KSTATUS i_cmdmgrJobExec(PJOB pjob) {
    int ret;
    PJOB_EXEC pexec;

    if(!svcKernelIsRunning()) {
        return KSTATUS_SVC_IS_STOPPING;
    }
    pexec = i_cmdmgrFindExec(pjob->_cmd);
    if(pexec == NULL)
        return KSTATUS_CMDMGR_COMMAND_NOT_FOUND;
    __atomic_add_fetch(&gCmdManager._activeJobs, 1, __ATOMIC_ACQUIRE);
    ret = pexec(pjob->_ts, pjob->_data, pjob->_dataSize);
    __atomic_sub_fetch(&gCmdManager._activeJobs, 1, __ATOMIC_RELEASE);
    if(ret != 0)
        //job should be rescheduled again
        //or job should be freed if rescheduled is not an option
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

KSTATUS i_cmdmgrExecutor(void* arg) {
    char buffer[100000];
    PJOB pjob = (PJOB)buffer;
    int ret;
    static struct timespec time_to_wait = {0, 0};

    SYSLOG(LOG_INFO, "[CMDMGR] Starting Job Executor");
    gCmdManager._cmdExecutorThreadId = pthread_self();
    while(svcKernelIsRunning()) {
        time_to_wait.tv_sec = time(NULL) + 1;
        ret = queue_read(gCmdManager._pjobQueue, pjob, &time_to_wait);
        if(ret > 0) {
            i_cmdmgrJobExec(pjob);
        } else if(ret == QUEUE_RET_ERROR) {
            SYSLOG(LOG_ERR, "[CMDMGR] Error during dequeue job");
        }
    }
    SYSLOG(LOG_INFO, "[CMDMGR] Stopping Job Executor");
    return KSTATUS_SUCCESS;
}

/* External API */

KSTATUS cmdmgrStart(void) {
    KSTATUS _status;
    SYSLOG(LOG_INFO, "[CMDMGR] Starting...");
    _status = dbExec(svcKernelGetDb(), cgCreateSchemaCmdList);
    if(!KSUCCESS(_status))
        return _status;
    gCmdManager._pjobQueue = queue_create(1000000);
    if(gCmdManager._pjobQueue == NULL)
        return KSTATUS_UNSUCCESS;
    gCmdManager._activeJobs = 0;
    _status = psmgrCreateThread("cmdmgrExecutor", PSMGR_THREAD_KERNEL, i_cmdmgrExecutor, NULL, NULL);
    return _status;
}

void cmdmgrStop(void) {
    SYSLOG(LOG_INFO, "[CMDMGR] Stopping...");
    queue_signal(gCmdManager._pjobQueue);
    psmgrWaitForThread(gCmdManager._cmdExecutorThreadId);
    queue_destroy(gCmdManager._pjobQueue);
}

void cmdmgrWaitForAllJobs(void) {
    while(__atomic_load_n(&gCmdManager._activeJobs, __ATOMIC_ACQUIRE) > 0) {
        sleep(1);
    }
}

KSTATUS cmdmgrAddCommand(const char* cmd, const char* description, PJOB_EXEC pexec, PJOB_CREATE pcreate, PJOB_DESTROY pdestroy, int version) {
	int rc;
	sqlite3_stmt *stmt;
	KSTATUS _status;

	rc = sqlite3_prepare_v2(svcKernelGetDb(), cgInsertCommand, -1, &stmt, 0);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to prepare cursor: %s", dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_bind_text(stmt, 1, cmd, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind command(%s): %s", cmd, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	rc = sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind description(%s): %s", description, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	rc = sqlite3_bind_int(stmt, 3, version);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind version(%d): %s", version, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	rc = sqlite3_bind_int64(stmt, 4, (sqlite_int64)pexec);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind pexec(%p): %s", pexec, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	rc = sqlite3_bind_int64(stmt, 5, (sqlite_int64)pcreate);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind pcreate(%p): %s", pcreate, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	rc = sqlite3_bind_int64(stmt, 6, (sqlite_int64)pdestroy);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind pdestroy(%p): %s", pdestroy, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	if(sqlite3_step(stmt) != SQLITE_DONE) {
		SYSLOG(LOG_ERR, "Failed to add new command=%s: %s", cmd, dbGetErrmsg(svcKernelGetDb()));
		_status = KSTATUS_UNSUCCESS;
		goto __cleanup;
	}
	rc = pcreate();
	if(rc != 0) {
		SYSLOG(LOG_ERR, "Failed to create command=%s: %d", cmd, rc);
		_status = KSTATUS_UNSUCCESS;
	}
__cleanup:
	if(!KSUCCESS(_status)) {
		pdestroy();
		return _status;
	}
	rc = sqlite3_finalize(stmt);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to clear cursor: %s", dbGetErrmsg(svcKernelGetDb()));
		pdestroy();
		return KSTATUS_UNSUCCESS;
	}
	SYSLOG(LOG_INFO, "Command %s(%p, %p, %p) added successfully", cmd, pexec, pcreate, pdestroy);
	return KSTATUS_SUCCESS;
}

KSTATUS cmdmgrJobPrepare(const char* cmd, void* pdata, size_t dataSize, struct timeval ts, PJOB* pjob) {
	PJOB pjob2;
	int cmdLength = strlen(cmd);
	DPRINTF("Allocate %zuB memory for command %s", dataSize, cmd);

	pjob2 = MALLOC2(JOB, 1, dataSize+cmdLength+1);
	if(pjob2 == NULL)
		return KSTATUS_OUT_OF_MEMORY;
	pjob2->_cmd = memoryPtrMove(pjob2, sizeof(JOB));
	memcpy(pjob2->_cmd, cmd, cmdLength);
	pjob2->_cmd[cmdLength] = '\0';
	pjob2->_ts = ts;
	pjob2->_data = memoryPtrMove(pjob2->_cmd, cmdLength+1);
	memcpy(pjob2->_data, pdata, dataSize);
	pjob2->_dataSize = dataSize;
	*pjob = pjob2;
	return KSTATUS_SUCCESS;
}

KSTATUS cmdmgrJobExec(PJOB pjob, JobMode mode) {
	KSTATUS _status = KSTATUS_UNSUCCESS;
	switch(mode) {
	//if mode == JobModeAsynchronous then function should back immediatelly and schedule job to future
	case JobModeAsynchronous:
		queue_write(gCmdManager._pjobQueue, pjob, pjob->_dataSize+sizeof(JOB), NULL);
		return KSTATUS_SUCCESS;
	//if mode == JobModeSynchronous then function should wait until execution is done
	case JobModeSynchronous:
		_status = i_cmdmgrJobExec(pjob);
		FREE(pjob);
	}
	return _status;
}

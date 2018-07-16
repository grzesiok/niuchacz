#include "cmd_manager.h"

static const char* cgCreateSchemaCmdList =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, version integer, routine integer"
		");";
static const char* cgInsertCommand =
		"insert into cmdmgr_cmdlist(code, description, version, routine)"
		"values (?, ?, ?, ?);";

typedef struct {
	queue_t *_pjobQueue;
	unsigned int _activeJobs;
} CMD_MANAGER, *PCMD_MANAGER;

CMD_MANAGER gCmdManager;

/* Internal API */

static PJOB_ROUTINE i_cmdmgrFindRoutine(const char* cmd) {
	int rc;
	sqlite3_stmt *stmt;
	PJOB_ROUTINE proutine;

	rc = sqlite3_prepare_v2(svcKernelGetDb(), "select routine from cmdmgr_cmdlist where code = ?;", -1, &stmt, 0);
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
    	proutine = (PJOB_ROUTINE)sqlite3_column_int64(stmt, 0);
        rc = sqlite3_finalize(stmt);
        if(rc != SQLITE_OK) {
        	SYSLOG(LOG_ERR, "Failed to clear cursor: %s", dbGetErrmsg(svcKernelGetDb()));
            return NULL;
        }
        DPRINTF("Routine %p found for cmd=%s", proutine, cmd);
        return proutine;
    }
	SYSLOG(LOG_ERR, "No command found for: %s", cmd);
    return NULL;
}

KSTATUS i_cmdmgrJobExec(PJOB pjob) {
	int ret;
	PJOB_ROUTINE proutine;

	if(!svcKernelIsRunning()) {
		return KSTATUS_SVC_IS_STOPPING;
	}
	proutine = i_cmdmgrFindRoutine(pjob->_cmd);
	if(proutine == NULL)
		return KSTATUS_CMDMGR_COMMAND_NOT_FOUND;
	__atomic_add_fetch(&gCmdManager._activeJobs, 1, __ATOMIC_ACQUIRE);
	ret = proutine(pjob->_ts, pjob->_data, pjob->_dataSize);
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
	gCmdManager._activeJobs = 0;
	if(gCmdManager._pjobQueue == NULL)
		return KSTATUS_UNSUCCESS;
	_status = psmgrCreateThread("cmdmgrExecutor", PSMGR_THREAD_KERNEL, i_cmdmgrExecutor, NULL, NULL);
	return _status;
}

void cmdmgrStop(void) {
	SYSLOG(LOG_INFO, "[CMDMGR] Stopping...");
	queue_destroy(gCmdManager._pjobQueue);
}

void cmdmgrWaitForAllJobs(void) {
	while(__atomic_load_n(&gCmdManager._activeJobs, __ATOMIC_ACQUIRE) > 0) {
		sleep(1);
	}
}

KSTATUS cmdmgrAddCommand(const char* cmd, const char* description, PJOB_ROUTINE proutine, int version) {
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(svcKernelGetDb(), cgInsertCommand, -1, &stmt, 0);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to prepare cursor: %s", dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_bind_text(stmt, 1, cmd, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind command(%s): %s", cmd, dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind description(%s): %s", description, dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_bind_int(stmt, 3, version);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind version(%d): %s", version, dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_bind_int64(stmt, 4, (sqlite_int64)proutine);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to bind routine(%p): %s", proutine, dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	if(sqlite3_step(stmt) != SQLITE_DONE) {
		SYSLOG(LOG_ERR, "Failed to add new command=%s: %s", cmd, dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_finalize(stmt);
	if(rc != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Failed to clear cursor: %s", dbGetErrmsg(svcKernelGetDb()));
		return KSTATUS_UNSUCCESS;
	}
	SYSLOG(LOG_INFO, "Command %s added successfully", cmd);
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

	DPRINTF("Execute job");
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

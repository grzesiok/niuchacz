#include "cmd_manager.h"

static const char* cgCreateSchema =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, version integer, routine integer"
		");";
static const char* cgInsertCommand =
		"insert into cmdmgr_cmdlist(code, description, version, routine)"
		"values (?, ?, ?, ?);";

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

/* External API */

KSTATUS cmdmgrStart(void) {
	KSTATUS _status;
	SYSLOG(LOG_INFO, "[CMDMGR] Starting...");
	_status = dbExec(svcKernelGetDb(), cgCreateSchema);
	return _status;
}

void cmdmgrStop(void) {
	SYSLOG(LOG_INFO, "[CMDMGR] Stopping...");
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
    DPRINTF("Command %s added successfully", cmd);
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
	PJOB_ROUTINE proutine;
	int ret;

	DPRINTF("Execute job");
	proutine = i_cmdmgrFindRoutine(pjob->_cmd);
	if(proutine == NULL)
		return KSTATUS_CMDMGR_COMMAND_NOT_FOUND;
	//if mode == JobModeAsynchronous then function should back immediatelly and schedule job to future
	//if mode == JobModeSynchronous then function should wait until execution is done
	ret = proutine(pjob->_ts, pjob->_data, pjob->_dataSize);
	if(ret != 0)
		//job should be rescheduled again
		//or job should be freed if rescheduled is not an option
		return KSTATUS_UNSUCCESS;
	FREE(pjob);
	return KSTATUS_SUCCESS;
}

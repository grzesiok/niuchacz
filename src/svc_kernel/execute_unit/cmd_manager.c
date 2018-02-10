#include "cmd_manager.h"

static const char* cgCreateSchema =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, routine integer, version integer"
		");";
static const char* cgInsertCommand =
		"insert into cmdmgr_cmdlist(code, description, routine, version)"
		"values (?, ?, ?, ?)";

/* Internal API */

static PCMD_ROUTINE i_cmdmgrFindRoutine(const char* command) {
	int rc;
	sqlite3_stmt *stmt;
	PCMD_ROUTINE proutine;

	rc = sqlite3_prepare_v2(svcKernelGetDb(), "select routine from cmdmgr_cmdlist where code = ?", -1, &stmt, 0);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to prepare cursor: %s", dbGetErrmsg(svcKernelGetDb()));
        return NULL;
    }

	rc = sqlite3_bind_text(stmt, 1, command, -1, SQLITE_STATIC);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to bind text(%s): %s\n", command, dbGetErrmsg(svcKernelGetDb()));
        return NULL;
    }

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW) {
    	proutine = (PCMD_ROUTINE)sqlite3_column_int64(stmt, 0);
        rc = sqlite3_finalize(stmt);
        if(rc != SQLITE_OK) {
        	syslog(LOG_ERR, "Failed to clear cursor: %s\n", dbGetErrmsg(svcKernelGetDb()));
            return NULL;
        }
        return proutine;
    }
	syslog(LOG_ERR, "No command found for: %s\n", command);
    return NULL;
}

/* External API */

KSTATUS cmdmgrStart(void) {
	KSTATUS _status;
	_status = dbExec(svcKernelGetDb(), cgCreateSchema);
	return _status;
}

void cmdmgrStop(void) {
}

KSTATUS cmdmgrAddCommand(const char* command, const char* description, PCMD_ROUTINE proutine, int version) {
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(svcKernelGetDb(), cgInsertCommand, -1, &stmt, 0);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to prepare cursor: %s", dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
	rc = sqlite3_bind_text(stmt, 1, command, -1, SQLITE_STATIC);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to bind command(%s): %s\n", command, dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
	rc = sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to bind description(%s): %s\n", description, dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
	rc = sqlite3_bind_int(stmt, 3, version);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to bind version(%d): %s\n", version, dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
	rc = sqlite3_bind_int64(stmt, 4, (sqlite_int64)proutine);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to bind routine(%p): %s\n", proutine, dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
    if(sqlite3_step(stmt) == SQLITE_DONE) {
    	syslog(LOG_ERR, "Failed to add new command=%s: %s\n", command, dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
    rc = sqlite3_finalize(stmt);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to clear cursor: %s\n", dbGetErrmsg(svcKernelGetDb()));
        return KSTATUS_UNSUCCESS;
    }
    return KSTATUS_SUCCESS;
}

KSTATUS cmdmgrExec(const char* command, const char* argv[], int argc) {
	PCMD_ROUTINE proutine;
	int ret;

	proutine = i_cmdmgrFindRoutine(command);
	if(proutine == NULL)
		return KSTATUS_CMDMGR_COMMAND_NOT_FOUND;
	ret = proutine(argv, argc);
	if(ret != 0)
		return KSTATUS_UNSUCCESS;
	return KSTATUS_SUCCESS;
}

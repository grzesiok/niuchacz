#include "cmd_manager.h"

static const char * cgCreateSchema =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, routine integer, version integer"
		");";

KSTATUS cmdmgrStart(void) {
	KSTATUS _status;
	_status = dbExec(svcKernelGetDb(), cgCreateSchema);
	return _status;
}

void cmdmgrStop(void) {
}

PCMD_ROUTINE cmdmgrFindRoutine(const char* pcmdText) {
	int rc;
	sqlite3_stmt *stmt;
	PCMD_ROUTINE proutine;

	rc = sqlite3_prepare_v2(svcKernelGetDb(), "select routine from cmdmgr_cmdlist where code = ?", -1, &stmt, 0);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to prepare cursor: %s", dbGetErrmsg(svcKernelGetDb()));
        return NULL;
    }

	rc = sqlite3_bind_text(stmt, 1, pcmdText, -1, SQLITE_STATIC);
    if(rc != SQLITE_OK) {
    	syslog(LOG_ERR, "Failed to bind text(%s): %s\n", pcmdText, dbGetErrmsg(svcKernelGetDb()));
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
	syslog(LOG_ERR, "No command found for: %s\n", pcmdText);
    return NULL;
}

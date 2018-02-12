#include "database.h"

stats_key g_statsKey_DbExecTime;

KSTATUS dbStart(const char* p_path, sqlite3** p_db)
{
	DPRINTF("dbStart(%s)", p_path);
	int  rc;
	KSTATUS _status;
	sqlite3* db;

	SYSLOG(LOG_INFO, "[DB] Starting(%s)...", p_path);
	_status = statsAlloc("db exec time", STATS_TYPE_SUM, &g_statsKey_DbExecTime);
	if(!KSUCCESS(_status)) {
		SYSLOG(LOG_ERR, "Error during allocation StatsKey!");
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_open(p_path, &db);
	_status = (rc) ? KSTATUS_DB_OPEN_ERROR : KSTATUS_SUCCESS;
	if(!KSUCCESS(_status))
		return _status;
	_status = dbExec(db, "PRAGMA journal_mode = WAL;");
	if(!KSUCCESS(_status))
		return _status;
	_status = dbExec(db, "PRAGMA synchronous = NORMAL;");
	if(!KSUCCESS(_status))
		return _status;
	SYSLOG(LOG_INFO, "[DB] Version: %s", sqlite3_libversion());
	if(rc) {
		*p_db = NULL;
		return KSTATUS_DB_OPEN_ERROR;
	}
	*p_db = db;
	return KSTATUS_SUCCESS;
}

void dbStop(sqlite3* db)
{
	DPRINTF("dbStop");
	SYSLOG(LOG_INFO, "[DB] Stopping...");
	sqlite3_close(db);
	statsFree(g_statsKey_DbExecTime);
}

KSTATUS dbExec(sqlite3* db, const char* stmt, ...)
{
	DPRINTF("dbExec(%s)", stmt);
	int ret;
	va_list args;
	char buff[512];
	char *errmsg;
	struct timespec startTime;

	va_start(args, stmt);
	ret = vsnprintf(buff, 512, stmt, args);
	va_end(args);
	startTime = timerStart();
	ret = sqlite3_exec(db, stmt, 0, 0, &errmsg);
	statsUpdate(g_statsKey_DbExecTime, timerStop(startTime));
    if(ret != SQLITE_OK) {
    	SYSLOG(LOG_ERR, "Error during processing query=(%s): %s!", stmt, errmsg);
    }
	return (ret != SQLITE_OK) ? KSTATUS_DB_EXEC_ERROR : KSTATUS_SUCCESS;
}

const char* dbGetErrmsg(sqlite3* db) {
	return sqlite3_errmsg(db);
}

bool dbBind_int64(bool isNotEmpty, sqlite3_stmt *pStmt, int i, sqlite_int64 iValue) {
	if(!isNotEmpty) {
		if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
			SYSLOG(LOG_ERR, "Error during binding NULL (%d).", i);
			return false;
		}
	} else {
		if(sqlite3_bind_int64(pStmt, i, iValue) != SQLITE_OK) {
			SYSLOG(LOG_ERR, "Error during binding variable (%d).", i);
			return false;
		}
	}
	return true;
}

bool dbBind_int(bool isNotEmpty, sqlite3_stmt *pStmt, int i, int iValue) {
	if(!isNotEmpty) {
		if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
			SYSLOG(LOG_ERR, "Error during binding NULL (%d).", i);
			return false;
		}
	} else {
		if(sqlite3_bind_int(pStmt, i, iValue) != SQLITE_OK) {
			SYSLOG(LOG_ERR, "Error during binding variable (%d).", i);
			return false;
		}
	}
	return true;
}

bool dbBind_text(bool isNotEmpty, sqlite3_stmt *pStmt, int i, const char *zData) {
	if(!isNotEmpty || zData == NULL) {
		if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
			SYSLOG(LOG_ERR, "Error during binding NULL (%d).", i);
			return false;
		}
	} else {
		if(sqlite3_bind_text(pStmt, i, zData, -1, SQLITE_STATIC) != SQLITE_OK) {
			SYSLOG(LOG_ERR, "Error during binding variable (%d).", i);
			return false;
		}
	}
	return true;
}

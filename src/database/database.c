#include "database.h"

static sqlite3* gFileDB;
static sqlite3* gMemoryDB;
stats_key g_statsKey_DbExecTime;

KSTATUS dbStart(const char* p_path)
{
	DPRINTF("dbStart\n");
	int  rc;
	KSTATUS _status;

	syslog(LOG_INFO, "[DB] Starting...\n");
	_status = statsAlloc("db exec time", STATS_TYPE_SUM, &g_statsKey_DbExecTime);
	if(!KSUCCESS(_status)) {
		syslog(LOG_ERR, "Error during allocation StatsKey!\n");
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_open(":memory:", &gMemoryDB);
	_status = (rc) ? KSTATUS_DB_OPEN_ERROR : KSTATUS_SUCCESS;
	if(!KSUCCESS(_status))
		return _status;
	rc = sqlite3_open(p_path, &gFileDB);
	_status = (rc) ? KSTATUS_DB_OPEN_ERROR : KSTATUS_SUCCESS;
	if(!KSUCCESS(_status))
		return _status;
	_status = dbExec(dbGetFileInstance(), "PRAGMA journal_mode = WAL;");
	if(!KSUCCESS(_status))
		return _status;
	_status = dbExec(dbGetFileInstance(), "PRAGMA synchronous = NORMAL;");
	if(!KSUCCESS(_status))
		return _status;
	syslog(LOG_INFO, "[DB] Version: %s\n", sqlite3_libversion());
	return (rc) ? KSTATUS_DB_OPEN_ERROR : KSTATUS_SUCCESS;
}

void dbStop(void)
{
	DPRINTF("dbStop\n");
	syslog(LOG_INFO, "[DB] Stopping...\n");
	sqlite3_close(gFileDB);
	sqlite3_close(gMemoryDB);
	statsFree(g_statsKey_DbExecTime);
}

sqlite3* dbGetFileInstance() {
	return gFileDB;
}

sqlite3* dbGetMemoryInstance() {
	return gMemoryDB;
}

KSTATUS dbExec(sqlite3* db, const char* stmt, ...)
{
	DPRINTF("dbExec(%s)\n", stmt);
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
    	syslog(LOG_ERR, "Error during insert data(%s)!\n", errmsg);
    }
	return (ret != SQLITE_OK) ? KSTATUS_DB_EXEC_ERROR : KSTATUS_SUCCESS;
}

const char* dbGetErrmsg(sqlite3* db) {
	return sqlite3_errmsg(db);
}

bool dbBind_int64(bool isNotEmpty, sqlite3_stmt *pStmt, int i, sqlite_int64 iValue) {
	if(!isNotEmpty) {
		if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
			syslog(LOG_ERR, "\nError during binding NULL (%d).", i);
			return false;
		}
	} else {
		if(sqlite3_bind_int64(pStmt, i, iValue) != SQLITE_OK) {
			syslog(LOG_ERR, "\nError during binding variable (%d).", i);
			return false;
		}
	}
	return true;
}

bool dbBind_int(bool isNotEmpty, sqlite3_stmt *pStmt, int i, int iValue) {
	if(!isNotEmpty) {
		if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
			syslog(LOG_ERR, "\nError during binding NULL (%d).", i);
			return false;
		}
	} else {
		if(sqlite3_bind_int(pStmt, i, iValue) != SQLITE_OK) {
			syslog(LOG_ERR, "\nError during binding variable (%d).", i);
			return false;
		}
	}
	return true;
}

bool dbBind_text(bool isNotEmpty, sqlite3_stmt *pStmt, int i, const char *zData) {
	if(!isNotEmpty || zData == NULL) {
		if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
			syslog(LOG_ERR, "\nError during binding NULL (%d).", i);
			return false;
		}
	} else {
		if(sqlite3_bind_text(pStmt, i, zData, -1, SQLITE_STATIC) != SQLITE_OK) {
			syslog(LOG_ERR, "\nError during binding variable (%d).", i);
			return false;
		}
	}
	return true;
}

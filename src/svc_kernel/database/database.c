#include "database.h"

stats_key g_statsKey_DbExecTime;

// Internal API

bool i_dbExecBindVariables(sqlite3_stmt *pStmt, va_list args, int bindCnt) {
    int i;
    for(i = 0;i < bindCnt;i++) {
        switch(va_arg(args, int)) {
        case DB_BIND_NULL:
            if(sqlite3_bind_null(pStmt, i+1) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_INT:
            if(sqlite3_bind_int(pStmt, i+1, va_arg(args, int)) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_INT64:
            if(sqlite3_bind_int64(pStmt, i+1, va_arg(args, sqlite_int64)) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_TEXT:
            if(sqlite3_bind_text(pStmt, i+1, va_arg(args, const char*), -1, SQLITE_STATIC) != SQLITE_OK) {
                return false;
            }
            break;
        default:
            SYSLOG(LOG_ERR, "[DB] Error during binding variable.");
            return false;
        }
    }
    return true;
}

int i_dbExecEmptyCallback(void* param, sqlite3_stmt* stmt) {
    return 0;
}

KSTATUS i_dbExec(sqlite3* db, const char* stmt, int bindCnt, int (*callback)(void*, sqlite3_stmt*), void* param, va_list args) {
    int rc;
    KSTATUS _status = KSTATUS_SUCCESS;
    struct timespec startTime;
    sqlite3_stmt *pStmt;
    unsigned int rowCount = 0;

    if(callback == NULL)
        callback = i_dbExecEmptyCallback;
    rc = sqlite3_prepare_v2(db, stmt, -1, &pStmt, 0);
    if(rc != SQLITE_OK) {
    	SYSLOG(LOG_ERR, "[DB] Failed to prepare cursor: %s", dbGetErrmsg(db));
        return KSTATUS_UNSUCCESS;
    }
    if(!i_dbExecBindVariables(pStmt, args, bindCnt)) {
    	SYSLOG(LOG_ERR, "[DB] Failed to bind variables to cursor: %s", dbGetErrmsg(db));
        return KSTATUS_UNSUCCESS;
    }
    while(1) {
        startTime = timerStart();
        rc = sqlite3_step(pStmt);
        statsUpdate(g_statsKey_DbExecTime, timerStop(startTime));
        if(rc == SQLITE_DONE || rc == SQLITE_ROW) {
            rowCount++;
        } else {
            SYSLOG(LOG_ERR, "[DB] Error during cursor executin: %s", dbGetErrmsg(db));
            break;
        }
        if(rc == SQLITE_DONE) {
            break;
        } else {
            rc = callback(param, pStmt);
            if(rc != 0) {
                SYSLOG(LOG_ERR, "[DB] Callback return with error!");
                break;
            }
        }
    }
    if(rowCount == 0) {
        SYSLOG(LOG_ERR, "[DB] Failed to execute cursor: %s", dbGetErrmsg(db));
        _status = KSTATUS_DB_EXEC_ERROR;
    }
    rc = sqlite3_finalize(pStmt);
    if(rc != SQLITE_OK) {
        SYSLOG(LOG_ERR, "[DB] Failed to clear cursor: %s", dbGetErrmsg(db));
        _status = KSTATUS_DB_EXEC_ERROR;
    }
    return _status;
}

// External API

KSTATUS dbStart(const char* p_path, sqlite3** p_db)
{
    int  rc;
    KSTATUS _status;
    sqlite3* db;

    SYSLOG(LOG_INFO, "[DB] Starting FileName(%s) Version(%s)...", p_path, sqlite3_libversion());
    _status = statsAlloc("db exec time", STATS_TYPE_SUM, &g_statsKey_DbExecTime);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB] Error during allocation StatsKey!");
        return KSTATUS_UNSUCCESS;
    }
    SYSLOG(LOG_INFO, "[DB] Openning FileName(%s) Version(%s)...", p_path, sqlite3_libversion());
    rc = sqlite3_open(p_path, &db);
    if(rc) {
        SYSLOG(LOG_ERR, "[DB] Error during opening DB(%s)!", p_path);
        *p_db = db;
        return KSTATUS_DB_OPEN_ERROR;
    }
    _status = dbExec(db, "PRAGMA journal_mode = WAL;", 0);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB] Error during enabling WAL journal_mode for DB(%s)!", p_path);
        return _status;
    }
    _status = dbExec(db, "PRAGMA synchronous = NORMAL;", 0);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB] Error during switching synchronous to NORMAL for DB(%s)!", p_path);
        return _status;
    }
    SYSLOG(LOG_INFO, "[DB] Opened FileName(%s) Version(%s)", p_path, sqlite3_libversion());
    *p_db = db;
    return KSTATUS_SUCCESS;
}

void dbStop(sqlite3* db)
{
    SYSLOG(LOG_INFO, "[DB] Stopping(%s)...", sqlite3_db_filename(db, "main"));
    sqlite3_close(db);
    statsFree(g_statsKey_DbExecTime);
}

KSTATUS dbExec(sqlite3* db, const char* stmt, int bindCnt, ...) {
    KSTATUS _status;
    va_list args;
    va_start(args, bindCnt);
    _status = i_dbExec(db, stmt, bindCnt, NULL, NULL, args);
    va_end(args);
    return _status;
}

KSTATUS dbExecQuery(sqlite3* db, const char* stmt, int bindCnt, int (*callback)(void*, sqlite3_stmt*), void* param, ...) {
    KSTATUS _status;
    va_list args;
    va_start(args, param);
    _status = i_dbExec(db, stmt, bindCnt, callback, param, args);
    va_end(args);
    return _status;
}

const char* dbGetErrmsg(sqlite3* db) {
    return sqlite3_errmsg(db);
}

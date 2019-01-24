#include "database.h"
#include "algorithms.h"

const char* gc_statsKey_DbExec = "db exec";
const char* gc_statsKey_DbExecFail = "db exec fail";
const char* gc_statsKey_DbPrepareTime = "db prepare time";
const char* gc_statsKey_DbBindTime = "db bind time";
const char* gc_statsKey_DbExecTime = "db exec time";
const char* gc_statsKey_DbFinalizeTime = "db finalize time";
const char* gc_statsKey_DbCallbackTime = "db callback time";

typedef struct {
	PDOUBLYLINKEDLIST _DBList;
} dbmgr_t;

static dbmgr_t g_dbCfg;
// Internal API

bool i_dbExecBindVariables(database_t *p_db, sqlite3_stmt *pStmt, va_list args, int bindCnt) {
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
        case DB_BIND_STEXT:
            if(sqlite3_bind_text(pStmt, i+1, va_arg(args, const char*), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
                return false;
            }
            break;
        default:
            SYSLOG(LOG_ERR, "[DB][%s] Error during binding variable.", p_db->_shortname_8b);
            return false;
        }
    }
    return true;
}

int i_dbExecEmptyCallback(void* param, sqlite3_stmt* stmt) {
    return 0;
}

KSTATUS i_dbExec(database_t* p_db, const char* stmt, int bindCnt, int (*callback)(void*, sqlite3_stmt*), void* param, va_list args) {
    int rc;
    KSTATUS _status = KSTATUS_SUCCESS;
    struct timespec startTime;
    sqlite3_stmt *pStmt;
    unsigned int rowCount = 0;
    unsigned long long l_DbPrepareTime = 0, l_DbBindTime = 0, l_DbExecTime = 0, l_DbCallbackTime = 0, l_DbFinalizeTime = 0;

    if(callback == NULL)
        callback = i_dbExecEmptyCallback;
    timerWatchStart(&startTime);
    rc = sqlite3_prepare_v2(p_db->_db, stmt, -1, &pStmt, 0);
    l_DbPrepareTime = timerWatchStop(startTime);
    if(rc != SQLITE_OK) {
    	SYSLOG(LOG_ERR, "[DB][%s] Failed to prepare cursor: %s", p_db->_shortname_8b, dbGetErrmsg(p_db));
        _status = KSTATUS_UNSUCCESS;
	goto __dbExec_cleanup;
    }
    timerWatchStart(&startTime);
    _status = (i_dbExecBindVariables(p_db, pStmt, args, bindCnt)) ? KSTATUS_SUCCESS : KSTATUS_UNSUCCESS;
    l_DbBindTime = timerWatchStop(startTime);
    if(!KSUCCESS(_status)) {
    	SYSLOG(LOG_ERR, "[DB][%s] Failed to bind variables to cursor: %s", p_db->_shortname_8b, dbGetErrmsg(p_db));
        goto __dbExec_cleanup;
    }
    while(1) {
        timerWatchStart(&startTime);
        rc = sqlite3_step(pStmt);
        l_DbExecTime += timerWatchStop(startTime);
        if(rc == SQLITE_DONE || rc == SQLITE_ROW) {
            rowCount++;
        } else {
            SYSLOG(LOG_ERR, "[DB][%s] Error during cursor executing: %s", p_db->_shortname_8b, dbGetErrmsg(p_db));
            break;
        }
        if(rc == SQLITE_DONE) {
            break;
        } else {
            timerWatchStart(&startTime);
            rc = callback(param, pStmt);
            l_DbCallbackTime += timerWatchStop(startTime);
            if(rc != 0) {
                SYSLOG(LOG_ERR, "[DB][%s] Callback return with error!", p_db->_shortname_8b);
                break;
            }
        }
    }
    if(rowCount == 0) {
        SYSLOG(LOG_ERR, "[DB][%s] Failed to execute cursor: %s", p_db->_shortname_8b, dbGetErrmsg(p_db));
        _status = KSTATUS_DB_EXEC_ERROR;
    }
    timerWatchStart(&startTime);
    rc = sqlite3_finalize(pStmt);
    l_DbFinalizeTime = timerWatchStop(startTime);
    if(rc != SQLITE_OK) {
        SYSLOG(LOG_ERR, "[DB][%s] Failed to clear cursor: %s", p_db->_shortname_8b, dbGetErrmsg(p_db));
        _status = KSTATUS_DB_EXEC_ERROR;
    }
__dbExec_cleanup:
    if(!KSUCCESS(_status)) {
        statsUpdate(&p_db->_statsEntry_DbExecFail, 1);
    }
    statsUpdate(&p_db->_statsEntry_DbExec, 1);
    statsUpdate(&p_db->_statsEntry_DbPrepareTime, l_DbPrepareTime);
    statsUpdate(&p_db->_statsEntry_DbBindTime, l_DbBindTime);
    statsUpdate(&p_db->_statsEntry_DbExecTime, l_DbExecTime);
    statsUpdate(&p_db->_statsEntry_DbCallbackTime, l_DbCallbackTime);
    statsUpdate(&p_db->_statsEntry_DbFinalizeTime, l_DbFinalizeTime);
    return _status;
}

// External API

KSTATUS dbmgrStart(void) {
    SYSLOG(LOG_INFO, "[DBMGR] Start...");
    g_dbCfg._DBList = doublylinkedlistAlloc();
    if(g_dbCfg._DBList == NULL)
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

void dbmgrStop(void) {
    SYSLOG(LOG_INFO, "[DBMGR] Cleaning up...");
    doublylinkedlistFreeDeletedEntries(g_dbCfg._DBList);
    doublylinkedlistFree(g_dbCfg._DBList);
}

KSTATUS dbStart(const char* p_path, const char* p_shortname_8b, database_t** p_out_db) {
    int  rc;
    KSTATUS _status;
    database_t db;
    database_t* p_db;

    memset(&db, 0, sizeof(database_t));
    strncpy(db._shortname_8b, p_shortname_8b, sizeof(db._shortname_8b)/sizeof(db._shortname_8b[0]));
    SYSLOG(LOG_INFO, "[DB][%s] Openning DB FileName(%s) Version(%s)...", p_shortname_8b, p_path, sqlite3_libversion());
    rc = sqlite3_open(p_path, &db._db);
    if(rc) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during opening DB(%s): %s!", p_shortname_8b, p_path, sqlite3_errmsg(db._db));
        return KSTATUS_DB_OPEN_ERROR;
    }
    *p_out_db = doublylinkedlistAdd(g_dbCfg._DBList, *((uint64_t*)db._shortname_8b), &db, sizeof(database_t));
    p_db = *p_out_db;
    SYSLOG(LOG_INFO, "[DB][%s] Starting FileName(%s) Version(%s)...", p_shortname_8b, p_path, sqlite3_libversion());
    p_db->_stats_list = statsCreate(p_db->_shortname_8b);
    if(p_db->_stats_list == NULL) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsList(%s)!", p_shortname_8b, p_shortname_8b);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbExec, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbExec);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbExec);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbExecFail, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbExecFail);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbExecFail);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbExecTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbExecTime);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbExecTime);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbPrepareTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbPrepareTime);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbPrepareTime);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbBindTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbBindTime);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbBindTime);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbFinalizeTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbFinalizeTime);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbFinalizeTime);
        return KSTATUS_UNSUCCESS;
    }
    _status = statsAlloc(p_db->_stats_list, gc_statsKey_DbCallbackTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbCallbackTime);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, gc_statsKey_DbCallbackTime);
        return KSTATUS_UNSUCCESS;
    }
    _status = dbExec(p_db, "PRAGMA journal_mode = WAL;", 0);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during enabling WAL journal_mode for DB(%s): %s!", p_db->_shortname_8b, p_path, dbGetErrmsg(p_db));
        return _status;
    }
    _status = dbExec(p_db, "PRAGMA synchronous = NORMAL;", 0);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during switching synchronous to NORMAL for DB(%s): %s!", p_db->_shortname_8b, p_path, dbGetErrmsg(p_db));
        return _status;
    }
    SYSLOG(LOG_INFO, "[DB][%s] Opened FileName(%s) Version(%s)", p_db->_shortname_8b, p_path, sqlite3_libversion());
    return KSTATUS_SUCCESS;
}

void dbStop(database_t* p_db)
{
    if(p_db == NULL)
        return;
    SYSLOG(LOG_INFO, "[DB][%s] Stopping...", p_db->_shortname_8b);
    doublylinkedlistDel(g_dbCfg._DBList, p_db);
    sqlite3_close(p_db->_db);
    SYSLOG(LOG_INFO, "[DB][%s][DbExec] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbExec));
    SYSLOG(LOG_INFO, "[DB][%s][DbExecFail] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbExecFail));
    SYSLOG(LOG_INFO, "[DB][%s][DbPrepareTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbPrepareTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbBindTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbBindTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbExecTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbExecTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbFinalizeTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbFinalizeTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbCallbackTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbCallbackTime));
    statsDestroy(p_db->_stats_list);
    doublylinkedlistRelease(p_db);
}

KSTATUS dbExec(database_t* p_db, const char* stmt, int bindCnt, ...) {
    KSTATUS _status;
    va_list args;
    va_start(args, bindCnt);
    _status = i_dbExec(p_db, stmt, bindCnt, NULL, NULL, args);
    va_end(args);
    return _status;
}

KSTATUS dbExecQuery(database_t* p_db, const char* stmt, int bindCnt, int (*callback)(void*, sqlite3_stmt*), void* param, ...) {
    KSTATUS _status;
    va_list args;
    va_start(args, param);
    _status = i_dbExec(p_db, stmt, bindCnt, callback, param, args);
    va_end(args);
    return _status;
}

KSTATUS dbTxnBegin(database_t* p_db) {
    sqlite3_exec(p_db->_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    return KSTATUS_SUCCESS;
}

KSTATUS dbTxnCommit(database_t* p_db) {
    sqlite3_exec(p_db->_db, "COMMIT;", NULL, NULL, NULL);
    return KSTATUS_SUCCESS;
}

KSTATUS dbTxnRollback(database_t* p_db) {
    sqlite3_exec(p_db->_db, "ROLLBACK;", NULL, NULL, NULL);
    return KSTATUS_SUCCESS;
}

const char* dbGetErrmsg(database_t* p_db) {
    return sqlite3_errmsg(p_db->_db);
}

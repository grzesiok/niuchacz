#include "database.h"
#include "../svc_kernel.h"
#include "algorithms.h"
#include "flags.h"

const char* gc_statsKey_DbExec = "db exec";
const char* gc_statsKey_DbExecFail = "db exec fail";
const char* gc_statsKey_DbPrepareTime = "db prepare time";
const char* gc_statsKey_DbBindTime = "db bind time";
const char* gc_statsKey_DbExecTime = "db exec time";
const char* gc_statsKey_DbFinalizeTime = "db finalize time";
const char* gc_statsKey_DbCallbackTime = "db callback time";
const char* gc_statsKey_DbTxnTime = "db txn time";
const char* gc_statsKey_DbTxnCommit = "db txn commit";
const char* gc_statsKey_DbTxnCommitFail = "db txn commit fail";
const char* gc_statsKey_DbTxnCommitTime = "db txn commit time";
const char* gc_statsKey_DbTxnRollback = "db txn rollback";
const char* gc_statsKey_DbTxnRollbackFail = "db txn rollback fail";
const char* gc_statsKey_DbTxnRollbackTime = "db txn rollback time";
const char* gc_statsKey_DbOpen = "db open";
const char* gc_statsKey_DbOpenTime = "db open time";

typedef struct {
	PDOUBLYLINKEDLIST _DBList;
} dbmgr_t;

static dbmgr_t g_dbCfg;
// Internal API

bool i_dbCheckState(database_t* p_db, uint32_t required_state) {
    if(p_db == NULL)
        return false;
    if(p_db->_flagsDBState != required_state)
        return false;
    return true;
}

bool i_dbExecBindVariables(database_t *p_db, sqlite3_stmt *pStmt, va_list args, int bindCnt) {
    int i;
    for(i = 1;i <= bindCnt;i++) {
        switch(va_arg(args, int)) {
        case DB_BIND_NULL:
            if(sqlite3_bind_null(pStmt, i) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_INT:
            if(sqlite3_bind_int(pStmt, i, va_arg(args, int)) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_INT64:
            if(sqlite3_bind_int64(pStmt, i, va_arg(args, sqlite_int64)) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_TEXT:
            if(sqlite3_bind_text(pStmt, i, va_arg(args, const char*), -1, SQLITE_STATIC) != SQLITE_OK) {
                return false;
            }
            break;
        case DB_BIND_STEXT:
            if(sqlite3_bind_text(pStmt, i, va_arg(args, const char*), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
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

    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_OPEN))
        return KSTATUS_DB_INCOMPATIBLE_STATE;
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

typedef struct {
    const char* _statsEntry_key;
    int _statsEntry_flags;
    stats_entry_t* _statsEntry_ptr;
} database_mount_init_stats_t;

KSTATUS i_dbMountInitStats(database_t* p_db, database_mount_init_stats_t* p_stats, int stats_num) {
    KSTATUS _status = KSTATUS_SUCCESS;
    int i;

    for(i = 0;i < stats_num;i++) {
        _status = statsAlloc(p_db->_stats_list, p_stats[i]._statsEntry_key, STATS_FLAGS_TYPE_SUM, p_stats[i]._statsEntry_ptr);
        if(!KSUCCESS(_status)) {
            SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsKey(%s)!", p_db->_shortname_8b, p_stats[i]._statsEntry_key);
            return KSTATUS_DB_MOUNT_ERROR;
        }
    }
    return _status;
}

KSTATUS i_dbMount(config_setting_t* p_instance_cfg) {
    KSTATUS _status;
    database_t db;
    database_t* p_db;
    const char *dbName = NULL, *fileName = NULL;
    int dropOnClose;

    if(!config_setting_lookup_string(p_instance_cfg, "dbName", &dbName)
    || !config_setting_lookup_string(p_instance_cfg, "fileName", &fileName)
    || !config_setting_lookup_bool(p_instance_cfg, "dropOnClose", &dropOnClose)) {
        SYSLOG(LOG_ERR, "[DBMGR] Settings are malformed !");
        return KSTATUS_DB_MOUNT_ERROR;
    }
    SYSLOG(LOG_INFO, "[DB][%s] Mounting FileName(%s) Version(%s)...", dbName, fileName, sqlite3_libversion());
    memset(&db, 0, sizeof(database_t));
    strncpy(db._shortname_8b, dbName, sizeof(db._shortname_8b)/sizeof(db._shortname_8b[0]));
    p_db = doublylinkedlistAdd(g_dbCfg._DBList, *((uint64_t*)db._shortname_8b), &db, sizeof(database_t));
    p_db->_flagsDBState = DB_FLAGS_STATE_MOUNTING;
    if(p_db == NULL) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during mounting DB(%s): internal structures can't be allocated!", p_db->_shortname_8b, p_db->_file_name);
        return KSTATUS_DB_MOUNT_ERROR;
    }
    p_db->_stats_list = statsCreate(p_db->_shortname_8b);
    if(p_db->_stats_list == NULL) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during allocation StatsList(%s)!", p_db->_shortname_8b, p_db->_shortname_8b);
        return KSTATUS_DB_MOUNT_ERROR;
    }
    database_mount_init_stats_t s_stats[] = {{gc_statsKey_DbExec, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbExec},
                                             {gc_statsKey_DbExecFail, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbExecFail},
                                             {gc_statsKey_DbExecTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbExecTime},
                                             {gc_statsKey_DbPrepareTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbPrepareTime},
                                             {gc_statsKey_DbBindTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbBindTime},
                                             {gc_statsKey_DbFinalizeTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbFinalizeTime},
                                             {gc_statsKey_DbCallbackTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbCallbackTime},
                                             {gc_statsKey_DbTxnTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnTime},
                                             {gc_statsKey_DbTxnCommit, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnCommit},
                                             {gc_statsKey_DbTxnCommitFail, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnCommitFail},
                                             {gc_statsKey_DbTxnCommitTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnCommitTime},
                                             {gc_statsKey_DbTxnRollback, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnRollback},
                                             {gc_statsKey_DbTxnRollbackFail, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnRollbackFail},
                                             {gc_statsKey_DbTxnRollbackTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbTxnRollbackTime},
                                             {gc_statsKey_DbOpen, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbOpen},
                                             {gc_statsKey_DbOpenTime, STATS_FLAGS_TYPE_SUM, &p_db->_statsEntry_DbOpenTime}};
    _status = i_dbMountInitStats(p_db, s_stats, sizeof(s_stats)/sizeof(s_stats[0]));
    if(!KSUCCESS(_status))
        return _status;
    p_db->_flagsDBState = DB_FLAGS_STATE_MOUNTED;
    if(dropOnClose) {
        p_db->_flagsDBDropOnClose = DB_FLAGS_TRUE;
    }
    p_db->_file_name = (char*)fileName;
    return _status;
}

void i_dbUmount(database_t* p_db) {
    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_MOUNTING) && !i_dbCheckState(p_db, DB_FLAGS_STATE_MOUNTED)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during umounting DB: wrong state!", p_db->_shortname_8b);
        return;
    }
    SYSLOG(LOG_INFO, "[DB][%s] Umounting...", p_db->_shortname_8b);
    doublylinkedlistDel(g_dbCfg._DBList, p_db);
    SYSLOG(LOG_INFO, "[DB][%s][DbExec] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbExec));
    SYSLOG(LOG_INFO, "[DB][%s][DbExecFail] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbExecFail));
    SYSLOG(LOG_INFO, "[DB][%s][DbPrepareTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbPrepareTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbBindTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbBindTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbExecTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbExecTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbFinalizeTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbFinalizeTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbCallbackTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbCallbackTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnCommit] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnCommit));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnCommitFAil] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnCommitFail));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnCommitTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnCommitTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnRollback] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnRollback));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnRollbackFail] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnRollbackFail));
    SYSLOG(LOG_INFO, "[DB][%s][DbTxnRollbackTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbTxnRollbackTime));
    SYSLOG(LOG_INFO, "[DB][%s][DbOpen] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbOpen));
    SYSLOG(LOG_INFO, "[DB][%s][DbOpenTime] = %llu", p_db->_shortname_8b, statsGetValue(&p_db->_statsEntry_DbOpenTime));
    statsDestroy(p_db->_stats_list);
    doublylinkedlistRelease(p_db);
}

// External API

KSTATUS dbmgrStart(void) {
    KSTATUS _status = KSTATUS_UNSUCCESS;
    int i;
    config_setting_t *instances_cfg;

    SYSLOG(LOG_INFO, "[DBMGR] Start...");
    instances_cfg = config_lookup(svcKernelGetCfg(), "DB.instances");
    if(instances_cfg == NULL)
        return _status;
    g_dbCfg._DBList = doublylinkedlistAlloc();
    if(g_dbCfg._DBList == NULL)
        return _status;
    int numInstances = config_setting_length(instances_cfg);
    for(i = 0;i < numInstances;i++) {
        config_setting_t *instance_cfg = config_setting_get_elem(instances_cfg, i);
        _status = i_dbMount(instance_cfg);
        if(!KSUCCESS(_status)) {
            return _status;
        }
    }
    return _status;
}

void dbmgrStop(void) {
    SYSLOG(LOG_INFO, "[DBMGR] Cleaning up...");
    doublylinkedlistFreeDeletedEntries(g_dbCfg._DBList);
    doublylinkedlistFree(g_dbCfg._DBList);
}

KSTATUS dbOpen(const char* p_shortname_8b, database_t** p_out_db) {
    int  rc;
    KSTATUS _status;
    database_t* p_db;
    struct timespec startTime;

    timerWatchStart(&startTime);
    SYSLOG(LOG_INFO, "[DB][%s] Opening DB Version(%s)...", p_shortname_8b, sqlite3_libversion());
    *p_out_db = doublylinkedlistFind(g_dbCfg._DBList, *((uint64_t*)p_shortname_8b));
    if(!*p_out_db) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during finding DB struct", p_shortname_8b);
        return KSTATUS_DB_OPEN_ERROR;
    }
    p_db = *p_out_db;
    rc = sqlite3_open(p_db->_file_name, &p_db->_db);
    if(rc) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during starting DB(%s): %s!", p_shortname_8b, p_db->_file_name, dbGetErrmsg(p_db));
        return KSTATUS_DB_OPEN_ERROR;
    }
    p_db->_flagsDBState = DB_FLAGS_STATE_OPEN;
    _status = dbExec(p_db, "PRAGMA journal_mode = WAL;", 0);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during enabling WAL journal_mode for DB: %s!", p_db->_shortname_8b, dbGetErrmsg(p_db));
        return _status;
    }
    _status = dbExec(p_db, "PRAGMA synchronous = NORMAL;", 0);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "[DB][%s] Error during switching synchronous to NORMAL for DB: %s!", p_db->_shortname_8b, dbGetErrmsg(p_db));
        return _status;
    }
    SYSLOG(LOG_INFO, "[DB][%s] Opened Version(%s)", p_db->_shortname_8b, sqlite3_libversion());
    statsUpdate(&p_db->_statsEntry_DbOpenTime, timerWatchStop(startTime));
    statsUpdate(&p_db->_statsEntry_DbOpen, 1);
    return KSTATUS_SUCCESS;
}

void dbClose(database_t* p_db) {
    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_OPEN))
        return;
    SYSLOG(LOG_INFO, "[DB][%s] Closing DB...", p_db->_shortname_8b);
    sqlite3_close(p_db->_db);
    p_db->_db = NULL;
    p_db->_flagsDBState = DB_FLAGS_STATE_MOUNTED;
    if(p_db->_flagsDBDropOnClose == DB_FLAGS_TRUE) {
        remove(p_db->_file_name);
    }
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
    KSTATUS _status;
    int rc;

    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_OPEN))
        return KSTATUS_DB_INCOMPATIBLE_STATE;
    rc = sqlite3_exec(p_db->_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if(rc == SQLITE_OK) {
        _status = KSTATUS_SUCCESS;
    } else {
        _status = KSTATUS_UNSUCCESS;
    }
    return _status;
}

KSTATUS dbTxnCommit(database_t* p_db) {
    KSTATUS _status;
    int rc;
    struct timespec startTime;
    unsigned long long l_DbTxnCommitTime;

    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_OPEN))
        return KSTATUS_DB_INCOMPATIBLE_STATE;
    timerWatchStart(&startTime);
    rc = sqlite3_exec(p_db->_db, "COMMIT;", NULL, NULL, NULL);
    l_DbTxnCommitTime = timerWatchStop(startTime);
    if(rc == SQLITE_OK) {
        statsUpdate(&p_db->_statsEntry_DbTxnCommit, 1);
        statsUpdate(&p_db->_statsEntry_DbTxnCommitTime, l_DbTxnCommitTime);
        _status = KSTATUS_SUCCESS;
    } else {
        statsUpdate(&p_db->_statsEntry_DbTxnCommitFail, 1);
        _status = KSTATUS_UNSUCCESS;
    }
    return _status;
}

KSTATUS dbTxnRollback(database_t* p_db) {
    KSTATUS _status;
    int rc;
    struct timespec startTime;
    unsigned long long l_DbTxnRollbackTime;

    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_OPEN))
        return KSTATUS_DB_INCOMPATIBLE_STATE;
    timerWatchStart(&startTime);
    rc = sqlite3_exec(p_db->_db, "ROLLBACK;", NULL, NULL, NULL);
    l_DbTxnRollbackTime = timerWatchStop(startTime);
    if(rc == SQLITE_OK) {
        statsUpdate(&p_db->_statsEntry_DbTxnRollback, 1);
        statsUpdate(&p_db->_statsEntry_DbTxnRollbackTime, l_DbTxnRollbackTime);
        _status = KSTATUS_SUCCESS;
    } else {
        statsUpdate(&p_db->_statsEntry_DbTxnRollback, 1);
        _status = KSTATUS_UNSUCCESS;
    }
    return _status;
}

const char* dbGetErrmsg(database_t* p_db) {
    if(!i_dbCheckState(p_db, DB_FLAGS_STATE_OPEN))
        return "DB is not open!";
    return sqlite3_errmsg(p_db->_db);
}

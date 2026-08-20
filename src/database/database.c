#include "database.h"
#include "sqlite3.h"
#include "algorithms/telemetry/telemetry.h"
#include "algorithms/dlist/dlist.h"
#include "algorithms/timer/timer.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

struct database_t {
    sqlite3 *_db;
    telemetry_list_t *_stats_list;
    db_state_t _state;
    bool _drop_on_close;
    char _shortname_8b[9];
    char *_file_name;
    
    /* FIXED: Swapped from concrete structures to pointers to support opaque type layout rules */
    telemetry_entry_t *_statsEntry_DbExec;
    telemetry_entry_t *_statsEntry_DbExecFail;
    telemetry_entry_t *_statsEntry_DbPrepareTime;
    telemetry_entry_t *_statsEntry_DbBindTime;
    telemetry_entry_t *_statsEntry_DbExecTime;
    telemetry_entry_t *_statsEntry_DbFinalizeTime;
    telemetry_entry_t *_statsEntry_DbCallbackTime;
    telemetry_entry_t *_statsEntry_DbTxnTime;
    telemetry_entry_t *_statsEntry_DbTxnCommit;
    telemetry_entry_t *_statsEntry_DbTxnCommitFail;
    telemetry_entry_t *_statsEntry_DbTxnCommitTime;
    telemetry_entry_t *_statsEntry_DbTxnRollback;
    telemetry_entry_t *_statsEntry_DbTxnRollbackFail;
    telemetry_entry_t *_statsEntry_DbTxnRollbackTime;
    telemetry_entry_t *_statsEntry_DbOpen;
    telemetry_entry_t *_statsEntry_DbOpenTime;
};

typedef struct {
    dlist_t *db_list;
    telemetry_list_t *global_telemetry_ctx;
} dbmgr_t;

static dbmgr_t g_db_cfg;

/* Local definitions of telemetry field mappings keys */
static const char* k_stat_exec          = "db.exec.count";
static const char* k_stat_exec_fail     = "db.exec.fail.count";
static const char* k_stat_prepare_time  = "db.prepare.time";
static const char* k_stat_bind_time     = "db.bind.time";
static const char* k_stat_exec_time     = "db.exec.time";
static const char* k_stat_finalize_time = "db.finalize.time";

// ====================================================================
// CORE EXECUTION ENGINES AND ISOLATED WORKERS
// ====================================================================

static bool i_db_check_state(const database_t *p_db, db_state_t required_state) {
    return p_db && (p_db->_state == required_state);
}

static int i_db_exec_internal(database_t *p_db, const char *stmt, int bind_cnt, int (*callback)(void*, void*), void *param, va_list args) {
    int rc;
    struct timespec start_time;
    sqlite3_stmt *p_stmt = NULL;
    unsigned int row_count = 0;
    uint64_t t_prepare = 0, t_bind = 0, t_exec = 0, t_callback = 0, t_finalize = 0;
    int process_status = 0;

    if (!i_db_check_state(p_db, DB_STATE_OPEN)) return -1;

    timerWatchStart(&start_time);
    rc = sqlite3_prepare_v2(p_db->_db, stmt, -1, &p_stmt, NULL);
    t_prepare = timerWatchStop(&start_time);
    
    if (rc != SQLITE_OK) {
        process_status = -1;
        goto __db_metrics_flush;
    }

    timerWatchStart(&start_time);
    for (int i = 1; i <= bind_cnt; i++) {
        int bind_type = va_arg(args, int);
        switch (bind_type) {
            case DB_BIND_NULL:  sqlite3_bind_null(p_stmt, i); break;
            case DB_BIND_INT:   sqlite3_bind_int(p_stmt, i, va_arg(args, int)); break;
            case DB_BIND_INT64: sqlite3_bind_int64(p_stmt, i, va_arg(args, sqlite3_int64)); break;
            case DB_BIND_TEXT:  sqlite3_bind_text(p_stmt, i, va_arg(args, const char*), -1, SQLITE_STATIC); break;
            case DB_BIND_STEXT: sqlite3_bind_text(p_stmt, i, va_arg(args, const char*), -1, SQLITE_TRANSIENT); break;
        }
    }
    t_bind = timerWatchStop(&start_time);

    while (1) {
        timerWatchStart(&start_time);
        rc = sqlite3_step(p_stmt);
        t_exec += timerWatchStop(&start_time);

        if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
            row_count++;
        } else {
            process_status = -1;
            break;
        }

        if (rc == SQLITE_DONE) break;
        
        if (callback != NULL) {
            timerWatchStart(&start_time);
            rc = callback(param, (void*)p_stmt);
            t_callback += timerWatchStop(&start_time);
            if (rc != 0) {
                process_status = -1;
                break;
            }
        }
    }

__db_metrics_flush:
    if (p_stmt != NULL) {
        timerWatchStart(&start_time);
        sqlite3_finalize(p_stmt);
        t_finalize = timerWatchStop(&start_time);
    }

    if (process_status != 0) telemetry_update(p_db->_statsEntry_DbExecFail, 1);
    telemetry_update(p_db->_statsEntry_DbExec, 1);
    telemetry_update(p_db->_statsEntry_DbPrepareTime, t_prepare);
    telemetry_update(p_db->_statsEntry_DbBindTime, t_bind);
    telemetry_update(p_db->_statsEntry_DbExecTime, t_exec);
    telemetry_update(p_db->_statsEntry_DbCallbackTime, t_callback);
    telemetry_update(p_db->_statsEntry_DbFinalizeTime, t_finalize);

    return process_status;
}

// ====================================================================
// PUBLIC API DISPATCH PLUGINS
// ====================================================================

int dbmgr_start(void *shared_telemetry_list) {
    g_db_cfg.global_telemetry_ctx = (telemetry_list_t*)shared_telemetry_list;
    g_db_cfg.db_list = (dlist_t*)dlist_alloc();
    return (g_db_cfg.db_list == NULL) ? -1 : 0;
}

void dbmgr_stop(void) {
    if (g_db_cfg.db_list != NULL) {
        dlist_free_deleted_entries(g_db_cfg.db_list);
        dlist_free(g_db_cfg.db_list);
        g_db_cfg.db_list = NULL;
    }
}

int db_open(const char *p_shortname_8b, const char *filename, database_t **p_db) {
    if (p_shortname_8b == NULL || filename == NULL || p_db == NULL) return -1;

    database_t *db = (database_t*)malloc(sizeof(database_t));
    if (db == NULL) return -1;
    memset(db, 0, sizeof(database_t));

    db->_state = DB_STATE_MOUNTING;
    db->_file_name = strdup(filename);
    strncpy(db->_shortname_8b, p_shortname_8b, sizeof(db->_shortname_8b) - 1);

    if (g_db_cfg.global_telemetry_ctx != NULL) {
        /* FIXED: Passed pointer references directly since memory addresses are now dynamic */
        telemetry_alloc(g_db_cfg.global_telemetry_ctx, k_stat_exec, TELEMETRY_TYPE_SUM, db->_statsEntry_DbExec);
        telemetry_alloc(g_db_cfg.global_telemetry_ctx, k_stat_exec_fail, TELEMETRY_TYPE_SUM, db->_statsEntry_DbExecFail);
        telemetry_alloc(g_db_cfg.global_telemetry_ctx, k_stat_prepare_time, TELEMETRY_TYPE_SUM, db->_statsEntry_DbPrepareTime);
        telemetry_alloc(g_db_cfg.global_telemetry_ctx, k_stat_bind_time, TELEMETRY_TYPE_SUM, db->_statsEntry_DbBindTime);
        telemetry_alloc(g_db_cfg.global_telemetry_ctx, k_stat_exec_time, TELEMETRY_TYPE_SUM, db->_statsEntry_DbExecTime);
        telemetry_alloc(g_db_cfg.global_telemetry_ctx, k_stat_finalize_time, TELEMETRY_TYPE_SUM, db->_statsEntry_DbFinalizeTime);
    }

    int rc = sqlite3_open(db->_file_name, &db->_db);
    if (rc != SQLITE_OK) {
        free(db->_file_name);
        free(db);
        return -1;
    }

    /* Configure busy timeout to reduce 'database is locked' errors when
     * multiple writers/readers contend for the DB file. Also enable WAL
     * mode to improve concurrent read/write performance. Failures are
     * non-fatal for compatibility, but best-effort is applied. */
    sqlite3_busy_timeout(db->_db, 5000); /* 5 seconds */
    sqlite3_exec(db->_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

    db->_state = DB_STATE_OPEN;
    *p_db = db;
    return 0;
}

void db_close(database_t *p_db) {
    if (p_db == NULL) return;
    if (p_db->_db != NULL) sqlite3_close(p_db->_db);
    if (p_db->_file_name) free(p_db->_file_name);
    free(p_db);
}

int db_exec(database_t *p_db, const char *stmt, int bind_cnt, ...) {
    va_list args;
    va_start(args, bind_cnt);
    int status = i_db_exec_internal(p_db, stmt, bind_cnt, NULL, NULL, args);
    va_end(args);
    return status;
}

int db_exec_query(database_t *p_db, const char *stmt, int bind_cnt, int (*callback)(void*, void*), void *param, ...) {
    va_list args;
    va_start(args, param);
    int status = i_db_exec_internal(p_db, stmt, bind_cnt, callback, param, args);
    va_end(args);
    return status;
}

int db_txn_begin(database_t *p_db) {
    if (!i_db_check_state(p_db, DB_STATE_OPEN)) return -1;
    return (sqlite3_exec(p_db->_db, "BEGIN TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK) ? 0 : -1;
}

int db_txn_commit(database_t *p_db) {
    if (!i_db_check_state(p_db, DB_STATE_OPEN)) return -1;
    return (sqlite3_exec(p_db->_db, "COMMIT;", NULL, NULL, NULL) == SQLITE_OK) ? 0 : -1;
}

int db_txn_rollback(database_t *p_db) {
    if (!i_db_check_state(p_db, DB_STATE_OPEN)) return -1;
    return (sqlite3_exec(p_db->_db, "ROLLBACK;", NULL, NULL, NULL) == SQLITE_OK) ? 0 : -1;
}

const char* db_get_errmsg(const database_t *p_db) {
    if (p_db == NULL || p_db->_db == NULL) return "Database interface connection closed!";
    return sqlite3_errmsg(p_db->_db);
}
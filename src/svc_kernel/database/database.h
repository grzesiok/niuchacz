#ifndef _DATABASE_H
#define _DATABASE_H
#include "sqlite3.h"
#include "svc_kernel/svc_status.h"
#include "svc_kernel/svc_statistics.h"

typedef struct {
    sqlite3* _db;
    char _shortname_8b[9];
    stats_key _statsKey_DbExec;
    stats_key _statsKey_DbExecFail;
    stats_key _statsKey_DbPrepareTime;
    stats_key _statsKey_DbBindTime;
    stats_key _statsKey_DbExecTime;
    stats_key _statsKey_DbFinalizeTime;
    stats_key _statsKey_DbCallbackTime;
} database_t;

KSTATUS dbmgrStart(void);
void dbmgrStop(void);
KSTATUS dbStart(const char* p_path, const char* p_shortname_8b, database_t** p_db);
void dbStop(database_t* p_db);
#define DB_BIND_NULL 0
#define DB_BIND_INT64 1
#define DB_BIND_INT 2
#define DB_BIND_TEXT 3
#define DB_BIND_STEXT 4
KSTATUS dbExec(database_t* p_db, const char* stmt, int bindCnt, ...);
KSTATUS dbExecQuery(database_t* p_db, const char* stmt, int bindCnt, int (*callback)(void*,sqlite3_stmt*), void* param, ...);
KSTATUS dbTxnBegin(database_t* p_db);
KSTATUS dbTxnCommit(database_t* p_db);
KSTATUS dbTxnRollback(database_t* p_db);
const char* dbGetErrmsg(database_t* p_db);

extern database_t* getNiuchaczPcapDB();
#endif /* _DATABASE_H */

#ifndef _DATABASE_H
#define _DATABASE_H
#include "sqlite3.h"
#include "svc_kernel/svc_status.h"
#include "svc_kernel/svc_statistics.h"
#include <libconfig.h>

typedef struct {
    sqlite3* _db;
    stats_list_t* _stats_list;
    char _shortname_8b[9];
    char* _file_name;
    stats_entry_t _statsEntry_DbExec;
    stats_entry_t _statsEntry_DbExecFail;
    stats_entry_t _statsEntry_DbPrepareTime;
    stats_entry_t _statsEntry_DbBindTime;
    stats_entry_t _statsEntry_DbExecTime;
    stats_entry_t _statsEntry_DbFinalizeTime;
    stats_entry_t _statsEntry_DbCallbackTime;
} database_t;

KSTATUS dbmgrStart(void);
void dbmgrStop(void);
KSTATUS dbOpen(const char* p_shortname_8b, database_t** p_db);
void dbClose(database_t* p_db);
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

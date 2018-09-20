#ifndef _DATABASE_H
#define _DATABASE_H
#include "sqlite3.h"
#include "svc_kernel/svc_kernel.h"

KSTATUS dbStart(const char* p_path, sqlite3** p_db);
void dbStop(sqlite3* db);
#define DB_BIND_NULL 0
#define DB_BIND_INT64 1
#define DB_BIND_INT 2
#define DB_BIND_TEXT 3
#define DB_BIND_STEXT 4
KSTATUS dbExec(sqlite3* db, const char* stmt, int bindCnt, ...);
KSTATUS dbExecQuery(sqlite3* db, const char* stmt, int bindCnt, int (*callback)(void*,sqlite3_stmt*), void* param, ...);
KSTATUS dbTxnBegin(sqlite3* db);
KSTATUS dbTxnCommit(sqlite3* db);
KSTATUS dbTxnRollback(sqlite3* db);
const char* dbGetErrmsg(sqlite3* db);

extern sqlite3* getNiuchaczPcapDB();
#endif /* _DATABASE_H */

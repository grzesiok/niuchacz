#ifndef _DATABASE_H
#define _DATABASE_H
#include "../../../sqlite/sqlite3.h"
#include "../svc_kernel.h"

KSTATUS dbStart(const char* p_path, sqlite3** p_db);
void dbStop(sqlite3* db);
KSTATUS dbExec(sqlite3* db, const char* stmt, ...);
const char* dbGetErrmsg(sqlite3* db);
//bind data
bool dbBind_int64(bool isNotEmpty, sqlite3_stmt *pStmt, int i, sqlite_int64 iValue);
bool dbBind_int(bool isNotEmpty, sqlite3_stmt *pStmt, int i, int iValue);
bool dbBind_text(bool isNotEmpty, sqlite3_stmt *pStmt, int i, const char *zData);

extern stats_key g_statsKey_DbExecTime;
#endif /* _DATABASE_H */

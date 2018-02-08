#ifndef _DATABASE_H
#define _DATABASE_H
#include "../svc_kernel/svc_kernel.h"
#include "../sqlite/sqlite3.h"

KSTATUS dbStart(const char* p_path);
void dbStop(void);
sqlite3* dbGetFileInstance();
sqlite3* dbGetMemoryInstance();
KSTATUS dbExec(sqlite3* db, const char* stmt, ...);
const char* dbGetErrmsg(sqlite3* db);
//bind data
bool dbBind_int64(bool isNotEmpty, sqlite3_stmt *pStmt, int i, sqlite_int64 iValue);
bool dbBind_int(bool isNotEmpty, sqlite3_stmt *pStmt, int i, int iValue);
bool dbBind_text(bool isNotEmpty, sqlite3_stmt *pStmt, int i, const char *zData);

extern stats_key g_statsKey_DbExecTime;
#endif

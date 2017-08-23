#ifndef _DATABASE_H
#define _DATABASE_H
#include "../svc_kernel/svc_kernel.h"
#include "../sqlite/sqlite3.h"

KSTATUS database_start(void);
void database_stop(void);
sqlite3* database_getinstance();
KSTATUS database_exec(const char* stmt, ...);
//bind data
bool database_bind_int64(bool isNotEmpty, sqlite3_stmt *pStmt, int i, sqlite_int64 iValue);
bool database_bind_int(bool isNotEmpty, sqlite3_stmt *pStmt, int i, int iValue);
bool database_bind_text(bool isNotEmpty, sqlite3_stmt *pStmt, int i, const char *zData);

extern stats_key g_statsKey_DbExecTime;
#endif

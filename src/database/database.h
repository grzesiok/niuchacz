#ifndef _DATABASE_H
#define _DATABASE_H
#include "../svc_kernel/svc_kernel.h"
#include "../sqlite/sqlite3.h"

KSTATUS database_start(void);
void database_stop(void);
sqlite3* database_getinstance();
KSTATUS database_exec(const char* stmt, ...);

#endif

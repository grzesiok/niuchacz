#ifndef _DATABASE_H
#define _DATABASE_H
#include "../sqlite/sqlite3.h"

extern sqlite3* db;

void select_stmt(const char* stmt);
void sql_stmt(const char* stmt);

#endif

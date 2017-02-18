#include "database.h"

static sqlite3* gDB;

KSTATUS database_start(void)
{
	DPRINTF("database_start\n");
	int  rc;
	rc = sqlite3_open("test.db", &gDB);
	return (rc) ? KSTATUS_DB_OPEN_ERROR : KSTATUS_SUCCESS;
}

void database_stop(void)
{
	DPRINTF("database_stop\n");
	sqlite3_close(gDB);
}

sqlite3* database_getinstance()
{
	return gDB;
}

KSTATUS database_exec(const char* stmt, ...)
{
	DPRINTF("database_exec(%s)\n", stmt);
	int ret;
	va_list args;
	char buff[512];
	char *errmsg;

	va_start(args, stmt);
	ret = vsnprintf(buff, 512, stmt, args);
	va_end(args);
	ret = sqlite3_exec(gDB, stmt, 0, 0, &errmsg);
	return (ret != SQLITE_OK) ? KSTATUS_DB_EXEC_ERROR : KSTATUS_SUCCESS;
}

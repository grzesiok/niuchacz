#include "database.h"

static sqlite3* gDB;
stats_key g_statsKey_DbExecTime;

KSTATUS database_start(void)
{
	DPRINTF("database_start\n");
	int  rc;
	KSTATUS _status;

	_status = statsAlloc("db exec time", STATS_TYPE_SUM, &g_statsKey_DbExecTime);
	if(!KSUCCESS(_status)) {
		printf("Error during allocation StatsKey!\n");
		return KSTATUS_UNSUCCESS;
	}
	rc = sqlite3_open("test.db", &gDB);
	return (rc) ? KSTATUS_DB_OPEN_ERROR : KSTATUS_SUCCESS;
}

void database_stop(void)
{
	DPRINTF("database_stop\n");
	sqlite3_close(gDB);
	statsFree(g_statsKey_DbExecTime);
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
	struct timespec startTime;

	va_start(args, stmt);
	ret = vsnprintf(buff, 512, stmt, args);
	va_end(args);
	startTime = timerStart();
	ret = sqlite3_exec(gDB, stmt, 0, 0, &errmsg);
	statsUpdate(g_statsKey_DbExecTime, timerStop(startTime));
	return (ret != SQLITE_OK) ? KSTATUS_DB_EXEC_ERROR : KSTATUS_SUCCESS;
}

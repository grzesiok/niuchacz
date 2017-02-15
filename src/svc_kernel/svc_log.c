#include "svc_log.h"
#include <fcntl.h>

typedef struct _LOGMGR
{
	LOCK_VAR;
	int _logfd;
} LOGMGR, *PLOGMGR;

static LOGMGR gLogmgrCfg;

KSTATUS svc_log_start(const char* pfile_name)
{
	gLogmgrCfg._logfd = open(pfile_name, O_APPEND | O_CREAT);
}

void svc_log_stop(void)
{
	close(gLogmgrCfg._logfd);
}

KSTATUS svc_log_fprintf(const char* format, ...)
{
	int bytes_written;
	char buff[512];
	va_list args;

	va_start(args, format);
	int bytes_to_write = vsnprintf(buff, 512, format, args);
	va_end(args);
	LOCK(&gLogmgrCfg, LOGMGR);
	bytes_written = write (gLogmgrCfg._logfd, buff, (size_t)bytes_to_write);
	UNLOCK(&gLogmgrCfg, LOGMGR);
	if(bytes_written != bytes_to_write)
		return KSTATUS_UNSUCCESS;
	return KSTATUS_SUCCESS;
}

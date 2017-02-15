#ifndef _SVC_LOG_H
#define _SVC_LOG_H
#include "../kernel.h"
KSTATUS svc_log_start(const char* pfile_name);
void svc_log_stop(void);
KSTATUS svc_log_fprintf(const char* format, ...);
#endif

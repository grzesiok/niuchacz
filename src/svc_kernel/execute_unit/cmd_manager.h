#ifndef _CMD_MANAGER_H
#define _CMD_MANAGER_H
#include "../svc_kernel.h"

typedef int (*PJOB_ROUTINE)(void* pdata, size_t dataSize);

typedef struct _JOB_T {
	char* _cmd;
	struct timeval _ts;
	void* _data;
	size_t _dataSize;
} JOB, *PJOB;

typedef enum _JobMode {
	JobModeSynchronous,
	JobModeAsynchronous
} JobMode;

KSTATUS cmdmgrStart(void);
void cmdmgrStop(void);
KSTATUS cmdmgrAddCommand(const char* cmd, const char* description, PJOB_ROUTINE proutine, int version);
KSTATUS cmdmgrJobPrepare(const char* cmd, void* pdata, size_t dataSize, struct timeval ts, PJOB* pjob);
KSTATUS cmdmgrJobExec(PJOB pjob, JobMode mode);
#endif /* _CMD_MANAGER_H */

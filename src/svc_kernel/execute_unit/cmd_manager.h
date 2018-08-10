#ifndef _CMD_MANAGER_H
#define _CMD_MANAGER_H
#include "../svc_kernel.h"

typedef int (*PJOB_EXEC)(struct timeval ts, void* pdata, size_t dataSize);
typedef int (*PJOB_CREATE)(void);
typedef int (*PJOB_DESTROY)(void);

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
void cmdmgrWaitForAllJobs(void);
KSTATUS cmdmgrAddCommand(const char* cmd, const char* description, PJOB_EXEC pexec, PJOB_CREATE pcreate, PJOB_DESTROY pdestroy, int version);
KSTATUS cmdmgrJobPrepare(const char* cmd, void* pdata, size_t dataSize, struct timeval ts, PJOB* pjob);
KSTATUS cmdmgrJobExec(PJOB pjob, JobMode mode);
#endif /* _CMD_MANAGER_H */

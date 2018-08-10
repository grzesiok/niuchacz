#include "../svc_kernel/svc_kernel.h"

int routineFirst(struct timeval ts, void* pdata, size_t dataSize) {
	DPRINTF("First Executed(%s, %zu)", (char*)pdata, dataSize);
	return 0;
}

int routineSecond(struct timeval ts, void* pdata, size_t dataSize) {
	DPRINTF("Second Executed(%s, %zu)", (char*)pdata, dataSize);
	return 0;
}

int dummyCreate() {
	return 0;
}

int dummyDestroy() {
	return 0;
}

int main(int argc, char* argv[]) {
	KSTATUS _status;
	PJOB pjob;
	struct timeval ts;

	_status = svcKernelInit(argv[1]);
	if(!KSUCCESS(_status))
		goto __exit;
	SYSLOG(LOG_INFO, "Environment prepared");
	_status = cmdmgrAddCommand("FIRST", "First command desc", routineFirst, dummyCreate, dummyDestroy, 1);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrAddCommand("SECOND", "Second command desc", routineSecond, dummyCreate, dummyDestroy, 1);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrJobPrepare("FIRST", "FIRST", strlen("FIRST")+1, ts, &pjob);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrJobExec(pjob, JobModeSynchronous);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrJobPrepare("SECOND", "SECOND", strlen("SECOND")+1, ts, &pjob);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrJobExec(pjob, JobModeSynchronous);
	if(!KSUCCESS(_status))
		goto __exit;
__exit:
	svcKernelStatus(SVC_KERNEL_STATUS_STOP_PENDING);
	svcKernelMainLoop();
	svcKernelExit(0);
	return 0;
}

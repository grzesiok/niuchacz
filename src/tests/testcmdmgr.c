#include "../svc_kernel/svc_kernel.h"

int routineFirst(void* pdata, size_t dataSize) {
	DPRINTF("First Executed(%s, %zu)", (char*)pdata, dataSize);
	return 0;
}

int routineSecond(void* pdata, size_t dataSize) {
	DPRINTF("Second Executed(%s, %zu)", (char*)pdata, dataSize);
	return 0;
}

int main(void) {
	KSTATUS _status;
	PJOB pjob;
	struct timeval ts;

	_status = svcKernelInit();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrAddCommand("FIRST", "First command desc", routineFirst, 1);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = cmdmgrAddCommand("SECOND", "Second command desc", routineSecond, 1);
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
	svcKernelExit(0);
	return 0;
}

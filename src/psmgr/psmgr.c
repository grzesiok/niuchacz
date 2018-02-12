#include "psmgr.h"
#include <errno.h>

#define PSMGR_MAXTHREADS 16

typedef struct _PSMGR {
	pthread_t _threadList[PSMGR_MAXTHREADS];
    volatile int _activeThreads;
} PSMGR, *PPSMGR;

typedef struct _PSMGR_THREAD_CTX {
	void *(*_p_routine) (void *);
	void *_p_arg;
} PSMGR_THREAD_CTX, *PPSMGR_THREAD_CTX;

static PSMGR g_PsmgrCfg;

static void* psmgrRoutine(void* p_arg) {
	PPSMGR_THREAD_CTX p_threadCtx = (PPSMGR_THREAD_CTX)p_arg;
	void* p_ret = p_threadCtx->_p_routine(p_threadCtx->_p_arg);
	return p_ret;
}

KSTATUS psmgrStart(void) {
	DPRINTF("psmgr_start");
	return KSTATUS_SUCCESS;
}

void psmgrStop(void) {
	DPRINTF("psmgr_stop");
}

void psmgrIdle(unsigned long long waitTimeInSec) {
	DPRINTF("psmgr_idle");
	void *p_ret;
	int i;
	struct timespec ts;
	int retStatus;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += waitTimeInSec;
	for(i = 0;i < g_PsmgrCfg._activeThreads;i++) {
		retStatus = pthread_timedjoin_np(g_PsmgrCfg._threadList[i], &p_ret, &ts);
		if(retStatus == ETIMEDOUT)
			return;
	}
}

KSTATUS psmgrCreateThread(void *(*p_startRoutine) (void *), void *p_arg) {
	DPRINTF("psmgr_create_thread");
	KSTATUS _status = KSTATUS_SUCCESS;
	PPSMGR_THREAD_CTX p_threadCtx;

    if(g_PsmgrCfg._activeThreads == PSMGR_MAXTHREADS)
    	return KSTATUS_UNSUCCESS;
    p_threadCtx = MALLOC(PSMGR_THREAD_CTX, 1);
    if(p_threadCtx == NULL)
    	return KSTATUS_OUT_OF_MEMORY;
    p_threadCtx->_p_arg = p_arg;
    p_threadCtx->_p_routine = p_startRoutine;
    if(g_PsmgrCfg._activeThreads == PSMGR_MAXTHREADS) {
    	_status = KSTATUS_UNSUCCESS;
    	goto __cleanup;
    }
    int ret = pthread_create(&g_PsmgrCfg._threadList[g_PsmgrCfg._activeThreads++], NULL, psmgrRoutine, p_threadCtx);
    if(ret != 0)
    	_status = KSTATUS_UNSUCCESS;
__cleanup:
    if(!KSUCCESS(_status))
    	FREE(p_threadCtx);
    return _status;
}

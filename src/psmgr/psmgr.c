#include "psmgr.h"

#define PSMGR_MAXTHREADS 16

typedef struct _PSMGR
{
	pthread_t _thread_list[PSMGR_MAXTHREADS];
    volatile int _active_threads;
} PSMGR, *PPSMGR;

typedef struct _PSMGR_THREAD_CTX
{
	void *(*_routine) (void *);
	void *_arg;
} PSMGR_THREAD_CTX, *PPSMGR_THREAD_CTX;

static PSMGR gPsmgrCfg;

static void* psmgr_routine(void* arg)
{
	PPSMGR_THREAD_CTX pthread_ctx = (PPSMGR_THREAD_CTX)arg;
	void* ret = pthread_ctx->_routine(pthread_ctx->_arg);
	return ret;
}

KSTATUS psmgr_start(void)
{
	DPRINTF("psmgr_start\n");
	memset(&gPsmgrCfg, 0, sizeof(PSMGR));
	return KSTATUS_SUCCESS;
}

void psmgr_stop(void)
{
	DPRINTF("psmgr_stop\n");
}

void psmgr_idle(void)
{
	DPRINTF("psmgr_idle\n");
	void *ret;
	int i;
	for(i = 0;i < gPsmgrCfg._active_threads;i++)
		pthread_join(gPsmgrCfg._thread_list[i], &ret);
}

KSTATUS psmgr_create_thread(void *(*start_routine) (void *), void *arg)
{
	DPRINTF("psmgr_create_thread\n");
	KSTATUS _status = KSTATUS_SUCCESS;
	PPSMGR_THREAD_CTX pthread_ctx;

    if(gPsmgrCfg._active_threads == PSMGR_MAXTHREADS)
    	return KSTATUS_UNSUCCESS;
    pthread_ctx = MALLOC(PSMGR_THREAD_CTX, 1);
    if(pthread_ctx == NULL)
    	return KSTATUS_OUT_OF_MEMORY;
    pthread_ctx->_arg = arg;
    pthread_ctx->_routine = start_routine;
    if(gPsmgrCfg._active_threads == PSMGR_MAXTHREADS)
    {
    	_status = KSTATUS_UNSUCCESS;
    	goto __cleanup;
    }
    int ret = pthread_create(&gPsmgrCfg._thread_list[gPsmgrCfg._active_threads++], NULL, psmgr_routine, pthread_ctx);
    if(ret != 0)
    	_status = KSTATUS_UNSUCCESS;
__cleanup:
    if(!KSUCCESS(_status))
    	FREE(pthread_ctx);
    return _status;
}

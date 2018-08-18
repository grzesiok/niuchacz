#include "psmgr.h"
#include <unistd.h>
#include <signal.h>
#include "algorithms.h"
#include <errno.h>

typedef struct _PSMGR_THREAD {
    pthread_t _threadId;
    pthread_cond_t _startExecCondVariable;
    pthread_mutex_t _mutex;
    union {
        unsigned long _flags;
        struct {
            unsigned long _threadType:2;
            unsigned long _threadStatus:1;
#define THREAD_STATUS_RUNNING 1
        };
    };
    const char* _fullThreadName;
    psmgr_execRoutine _p_execRoutine;
    psmgr_cancelRoutine _p_cancelRoutine;
    void *_p_arg;
} PSMGR_THREAD, *PPSMGR_THREAD;

typedef struct _PSMGR {
	PDOUBLYLINKEDLIST _threadList;
} PSMGR, *PPSMGR;

static PSMGR g_psmgrCfg;

// internal API
static void i_psmgrHandleMainTerminate(int signo) {
    mpp_printf("psmgrHandleMainTerminate(pid=%lu, %d)\n", pthread_self(), signo);
    svcKernelStatus(SVC_KERNEL_STATUS_STOP_PENDING);
    /*if(signo == SIGHUP) TODO: reload configuration file */

}

static void i_psmgrCancelRoutineEmpty(void* ptr) {
}

typedef struct {
    int _signo;
    struct sigaction _sigcfg;
} signal_handlers_t;

static const signal_handlers_t g_psmgrHandledSignalsMainNew[] = {
    {SIGCHLD, (struct sigaction){}},//.sa_handler = i_psmgrHandleChildTerminate, .sa_flags = SA_SIGINFO}},
    {SIGTERM, (struct sigaction){.sa_handler = i_psmgrHandleMainTerminate, .sa_flags = SA_SIGINFO}},
    {SIGINT, (struct sigaction){.sa_handler = i_psmgrHandleMainTerminate, .sa_flags = SA_SIGINFO}}
};
static signal_handlers_t g_psmgrHandledSignalsMainOld[] = {
    {SIGCHLD, (struct sigaction){}},
    {SIGTERM, (struct sigaction){}},
    {SIGINT, (struct sigaction){}}
};

static void* i_psmgrExecRoutine(void* p_arg) {
    int ret;
    PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)p_arg, p_threadCtxOld = (PPSMGR_THREAD)p_arg;

    SYSLOG(LOG_INFO, "[PSMGR] INIT threadName=%s threadId=%lu p_execRoutine=%p", p_threadCtx->_fullThreadName, pthread_self(), p_threadCtx->_p_execRoutine);
    p_threadCtx = doublylinkedlistAdd(g_psmgrCfg._threadList, pthread_self(), p_threadCtx, sizeof(PSMGR_THREAD));
    pthread_mutex_lock(&p_threadCtxOld->_mutex);
    p_threadCtxOld->_threadStatus = THREAD_STATUS_RUNNING;
    pthread_cond_broadcast(&p_threadCtxOld->_startExecCondVariable);
    pthread_mutex_unlock(&p_threadCtxOld->_mutex);
    pthread_mutex_destroy(&p_threadCtxOld->_mutex);
    pthread_cond_destroy(&p_threadCtxOld->_startExecCondVariable);
    FREE(p_threadCtxOld);
    if(svcKernelIsRunning()) {
        SYSLOG(LOG_INFO, "[PSMGR] ENTER threadName=%s threadId=%lu p_execRoutine=%p", p_threadCtx->_fullThreadName, pthread_self(), p_threadCtx->_p_execRoutine);
        ret = p_threadCtx->_p_execRoutine(p_threadCtx->_p_arg);
        SYSLOG(LOG_INFO, "[PSMGR] RET threadName=%s threadId=%lu p_execRoutine=%p ret=%d", p_threadCtx->_fullThreadName, pthread_self(), p_threadCtx->_p_execRoutine, ret);
    }
    //release lock from execRoutine
    doublylinkedlistDel(g_psmgrCfg._threadList, p_threadCtx);
    SYSLOG(LOG_INFO, "[PSMGR] EXIT threadName=%s threadId=%lu p_execRoutine=%p", p_threadCtx->_fullThreadName, pthread_self(), p_threadCtx->_p_execRoutine);
    //free object
    doublylinkedlistRelease(p_threadCtx);
    return (void*)ret;
}

static void i_psmgrDumpList(void) {
    char buff[4096];
    PDOUBLYLINKEDLIST_QUERY pquery;
    const size_t current_size = sizeof(buff);
    size_t required_size;

    SYSLOG(LOG_DEBUG, "[PSMGR] DUMP_START");
    pquery = (PDOUBLYLINKEDLIST_QUERY)buff;
    required_size = current_size;
    if(!doublylinkedlistQuery(g_psmgrCfg._threadList, pquery, &required_size)) {
        SYSLOG(LOG_DEBUG, "[PSMGR] Too much entries!");
    }
    SYSLOG(LOG_DEBUG, "[PSMGR] curr_size=%lu required_size=%lu", current_size, required_size);
    while(!doublylinkedlistQueryIsEnd(pquery)) {
        SYSLOG(LOG_DEBUG, "[PSMGR] Entry key=%lu references=%u isDeleted=%c size=%lu", pquery->_key, pquery->_references, (pquery->_isDeleted) ? 'Y' : 'N', pquery->_size);
        PPSMGR_THREAD pthread = (PPSMGR_THREAD)pquery->_p_userData;
        SYSLOG(LOG_DEBUG, "[PSMGR] Entry_thread threadId=%lu fullThreadName=%s", pthread->_threadId, pthread->_fullThreadName);
        pquery = doublylinkedlistQueryNext(pquery);
    }
    SYSLOG(LOG_DEBUG, "[PSMGR] DUMP_STOP");
}

// external API

KSTATUS psmgrStart(void) {
    int i;
    bool all_signals_handled = true;
    SYSLOG(LOG_INFO, "[PSMGR] Starting ...");
    for(i = 0;i < sizeof(g_psmgrHandledSignalsMainNew)/sizeof(g_psmgrHandledSignalsMainNew[0]);i++) {
        if(sigaction(g_psmgrHandledSignalsMainNew[i]._signo, &g_psmgrHandledSignalsMainNew[i]._sigcfg, &g_psmgrHandledSignalsMainOld[i]._sigcfg) != 0)
            all_signals_handled = false;
    }
    if(!all_signals_handled)
        return KSTATUS_UNSUCCESS;
    g_psmgrCfg._threadList = doublylinkedlistAlloc();
    if(g_psmgrCfg._threadList == NULL)
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

void psmgrStop(void) {
    int i;
    i_psmgrDumpList();
    SYSLOG(LOG_INFO, "[PSMGR] Stopping ...");
    for(i = 0;i < sizeof(g_psmgrHandledSignalsMainOld)/sizeof(g_psmgrHandledSignalsMainOld[0]);i++) {
        sigaction(g_psmgrHandledSignalsMainOld[i]._signo, &g_psmgrHandledSignalsMainOld[i]._sigcfg, NULL);
    }
    SYSLOG(LOG_INFO, "[PSMGR] Cleaning up...");
    doublylinkedlistFreeDeletedEntries(g_psmgrCfg._threadList);
    doublylinkedlistFree(g_psmgrCfg._threadList);
}

KSTATUS psmgrIdle(unsigned long long waitTimeInSec) {
    if(!doublylinkedlistIsEmpty(g_psmgrCfg._threadList)) {
        sleep(waitTimeInSec);
    }
    return KSTATUS_SUCCESS;
}

void psmgrStopUserThreads(void) {
    SYSLOG(LOG_INFO, "[PSMGR] Stopping User Threads ...");
    while(!doublylinkedlistIsEmpty(g_psmgrCfg._threadList)) {
        /*PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)doublylinkedlistGetFirst(g_psmgrCfg._threadList);
        if(p_threadCtx->_threadType == PSMGR_THREAD_USER) {
            //SYSLOG(LOG_INFO, "[PSMGR] Killing pid=%d ...", p_threadCtx->_pid);
            //kill(p_threadCtx->_pid, SIGTERM);
        }
        doublylinkedlistRelease(p_threadCtx);*/
        sleep(1);
    }
}

KSTATUS psmgrCreateThread(const char* c_threadName, int threadType, psmgr_execRoutine p_execRoutine, psmgr_cancelRoutine p_cancelRoutine, void *p_arg) {
    PPSMGR_THREAD p_threadCtx;
    int ret;

    p_threadCtx = MALLOC(PSMGR_THREAD, 1);
    if(p_threadCtx == NULL)
        return KSTATUS_OUT_OF_MEMORY;
    p_threadCtx->_p_arg = p_arg;
    p_threadCtx->_p_execRoutine = p_execRoutine;
    if(p_cancelRoutine != NULL) {
        p_threadCtx->_p_cancelRoutine = p_cancelRoutine;
    } else {
        p_threadCtx->_p_cancelRoutine = i_psmgrCancelRoutineEmpty;
    }
    p_threadCtx->_fullThreadName = c_threadName;
    p_threadCtx->_threadType = threadType;
    pthread_cond_init(&p_threadCtx->_startExecCondVariable, NULL);
    pthread_mutex_init(&p_threadCtx->_mutex, NULL);
    pthread_mutex_lock(&p_threadCtx->_mutex);
    ret = pthread_create(&p_threadCtx->_threadId, NULL, i_psmgrExecRoutine, p_threadCtx);
    SYSLOG(LOG_INFO, "[PSMGR] CREATE threadName=%s threadId=%lu p_execRoutine=%p p_cancelRoutine=%p", c_threadName, p_threadCtx->_threadId, p_execRoutine, p_cancelRoutine);
    if(ret != 0) {
        pthread_mutex_unlock(&p_threadCtx->_mutex);
        pthread_cond_destroy(&p_threadCtx->_startExecCondVariable);
        pthread_mutex_destroy(&p_threadCtx->_mutex);
        FREE(p_threadCtx);
        return KSTATUS_UNSUCCESS;
    }
    while(p_threadCtx->_threadStatus != THREAD_STATUS_RUNNING)
        ret = pthread_cond_wait(&p_threadCtx->_startExecCondVariable, &p_threadCtx->_mutex);
    if(ret != 0) {
        pthread_mutex_unlock(&p_threadCtx->_mutex);
        pthread_cond_destroy(&p_threadCtx->_startExecCondVariable);
        pthread_mutex_destroy(&p_threadCtx->_mutex);
        FREE(p_threadCtx);
        return KSTATUS_UNSUCCESS;
    }
    pthread_mutex_unlock(&p_threadCtx->_mutex);
    return KSTATUS_SUCCESS;
}

KSTATUS psmgrWaitForThread(pthread_t threadId) {
    int ret;
    static struct timespec time_to_wait = {0, 0};

    while(ret != ETIMEDOUT) {
        time_to_wait.tv_sec = time(NULL) + 1;
        ret = pthread_timedjoin_np(threadId, NULL, &time_to_wait);
        if(ret == 0)
            return KSTATUS_SUCCESS;
    }
    return KSTATUS_UNSUCCESS;
}

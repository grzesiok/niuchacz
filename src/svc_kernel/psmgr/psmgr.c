#include "psmgr.h"
#include <errno.h>
#include <linux/sched.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "algorithms.h"

typedef struct _PSMGR_THREAD {
	pid_t _pid;
	pid_t _ppid;
	const char* _fullThreadName;
	int _threadType;
	psmgr_execRoutine _p_execRoutine;
	psmgr_cancelRoutine _p_cancelRoutine;
	void *_p_arg;
	void *_p_stack;
} PSMGR_THREAD, *PPSMGR_THREAD;

typedef struct _PSMGR {
	PDOUBLYLINKEDLIST _threadList;
} PSMGR, *PPSMGR;

static PSMGR g_psmgrCfg;

#define STACK_SIZE (64 * 1024)
#define STACK_ALLOC() (void*)malloc(STACK_SIZE)
#define STACK_FREE(var) free(var)

// internal API

static void i_psmgrHandleChildTerminate(int signo) {
    pid_t pid;
    int status;
    struct rusage child_rusage;
    mpp_printf("psmgrChildTerminateHanlder(pid=%d, %d)\n", getpid(), signo);
    pid = wait3(&status, WNOHANG, &child_rusage);
}

static void i_psmgrCancelRoutineEmpty(void* ptr) {
}

static void i_psmgrCancelRoutine(int signo) {
    mpp_printf("psmgrCancelHanlder(pid=%d, %d)\n", getpid(), signo);
    PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)doublylinkedlistFind(g_psmgrCfg._threadList, getpid());
//    SYSLOG(LOG_INFO, "[PSMGR] ENTER threadName=%s ppid=%u pid=%u p_cancelRoutine=%p signo=%d", p_threadCtx->_fullThreadName, getppid(), getpid(), p_threadCtx->_p_cancelRoutine, signo);
    p_threadCtx->_p_cancelRoutine(p_threadCtx->_p_arg);
//    SYSLOG(LOG_INFO, "[PSMGR] RET threadName=%s ppid=%u pid=%u p_cancelRoutine=%p signo=%d", p_threadCtx->_fullThreadName, getppid(), getpid(), p_threadCtx->_p_cancelRoutine, signo);
    //release lock from execRoutine
    doublylinkedlistRelease(p_threadCtx);
    //release lock from cancelRoutine
    doublylinkedlistDel(g_psmgrCfg._threadList, p_threadCtx);
    STACK_FREE(p_threadCtx->_p_stack);
    //free object
    doublylinkedlistRelease(p_threadCtx);
    _exit(1);
}

typedef struct {
    int _signo;
    void (*_sigHandler)(int);
} signal_handlers_t;

static const signal_handlers_t g_psmgrHandledSignals[] = {
    {SIGILL, i_psmgrCancelRoutine},
    {SIGFPE, i_psmgrCancelRoutine},
    {SIGSEGV, i_psmgrCancelRoutine},
    {SIGCHLD, i_psmgrHandleChildTerminate},
    {SIGTERM, SIG_IGN},
    {SIGINT, SIG_IGN},
    {SIGABRT, SIG_IGN}
};
static int i_psmgrExecRoutine(void* p_arg) {
    int i = 0, ret;
    bool all_signals_handled = true;
    PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)p_arg;
    
    SYSLOG(LOG_INFO, "[PSMGR] INIT threadName=%s ppid=%u pid=%u p_execRoutine=%p", p_threadCtx->_fullThreadName, getppid(), getpid(), p_threadCtx->_p_execRoutine);
    for(i = 0;i < sizeof(g_psmgrHandledSignals)/sizeof(g_psmgrHandledSignals[0]);i++) {
        if(signal(g_psmgrHandledSignals[i]._signo, g_psmgrHandledSignals[i]._sigHandler) == SIG_ERR)
            all_signals_handled = false;
    }
    p_threadCtx->_pid = getpid();
    p_threadCtx->_ppid = getppid();
    p_threadCtx = doublylinkedlistAdd(g_psmgrCfg._threadList, getpid(), p_threadCtx, sizeof(PSMGR_THREAD));
    if(svcKernelIsRunning() && all_signals_handled) {
        SYSLOG(LOG_INFO, "[PSMGR] ENTER threadName=%s ppid=%u pid=%u p_execRoutine=%p", p_threadCtx->_fullThreadName, getppid(), getpid(), p_threadCtx->_p_execRoutine);
        ret = p_threadCtx->_p_execRoutine(p_threadCtx->_p_arg);
        SYSLOG(LOG_INFO, "[PSMGR] RET threadName=%s ppid=%u pid=%u p_execRoutine=%p ret=%d", p_threadCtx->_fullThreadName, getppid(), getpid(), p_threadCtx->_p_execRoutine, ret);
    }
    for(i = 0;i < sizeof(g_psmgrHandledSignals)/sizeof(g_psmgrHandledSignals[0]);i++) {
        signal(g_psmgrHandledSignals[i]._signo, SIG_DFL);
    }
    //release lock from execRoutine
    doublylinkedlistDel(g_psmgrCfg._threadList, p_threadCtx);
    SYSLOG(LOG_INFO, "[PSMGR] EXIT threadName=%s ppid=%u pid=%u p_execRoutine=%p", p_threadCtx->_fullThreadName, getppid(), getpid(), p_threadCtx->_p_execRoutine);
    //free object
    STACK_FREE(p_threadCtx->_p_stack);
    doublylinkedlistRelease(p_threadCtx);
    return ret;
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
        SYSLOG(LOG_DEBUG, "[PSMGR] Entry_thread pid=%d ppid=%d fullThreadName=%s", pthread->_pid, pthread->_ppid, pthread->_fullThreadName);
        pquery = doublylinkedlistQueryNext(pquery);
    }
    SYSLOG(LOG_DEBUG, "[PSMGR] DUMP_STOP");
}

// external API

KSTATUS psmgrStart(void) {
    SYSLOG(LOG_INFO, "[PSMGR] Starting ...");
    //if(signal(SIGCHLD, i_psmgrHandleChildTerminate) == SIG_ERR)
    //    return KSTATUS_UNSUCCESS;
    g_psmgrCfg._threadList = doublylinkedlistAlloc();
    if(g_psmgrCfg._threadList == NULL)
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

void psmgrStop(void) {
    i_psmgrDumpList();
    SYSLOG(LOG_INFO, "[PSMGR] Stopping ...");
    //signal(SIGCHLD, SIG_DFL);
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
    //KSTATUS _status = KSTATUS_UNSUCCESS;
    PPSMGR_THREAD p_threadCtx;
    pid_t pid;

    p_threadCtx = MALLOC(PSMGR_THREAD, 1);
    if(p_threadCtx == NULL) {
        return KSTATUS_OUT_OF_MEMORY;
    }
    p_threadCtx->_p_arg = p_arg;
    p_threadCtx->_p_execRoutine = p_execRoutine;
    if(p_cancelRoutine != NULL) {
        p_threadCtx->_p_cancelRoutine = p_cancelRoutine;
    } else {
        p_threadCtx->_p_cancelRoutine = i_psmgrCancelRoutineEmpty;
    }
    p_threadCtx->_fullThreadName = c_threadName;
    p_threadCtx->_threadType = threadType;
    p_threadCtx->_p_stack = STACK_ALLOC();
    if(p_threadCtx->_p_stack == NULL) {
        FREE(p_threadCtx);
        return KSTATUS_OUT_OF_MEMORY;
    }
    //CLONE_VM - common memory space for parent and child process
    //CLONE_FS - common file system information (This includes the root of the file system, the current working directory, and the umask)
    //CLONE_FILES - common file descriptor table
    pid = clone(i_psmgrExecRoutine, p_threadCtx->_p_stack + STACK_SIZE-1, CLONE_FILES | CLONE_VM | CLONE_FS, p_threadCtx);
    if(pid != -1) {
         SYSLOG(LOG_INFO, "[PSMGR] CREATE threadName=%s pid=%u ppid=%u p_execRoutine=%p p_cancelRoutine=%p", c_threadName, pid, getpid(), p_execRoutine, p_cancelRoutine);
         /*if(waitpid(pid, NULL, __WALL) == -1) {
                  DPRINTF(TEXT("errno %u"), errno);
         } else _status = KSTATUS_SUCCESS;*/
    } else {
        FREE(p_threadCtx);
     	STACK_FREE(p_threadCtx->_p_stack);
    }
    return KSTATUS_SUCCESS;
}

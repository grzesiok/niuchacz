#include "psmgr.h"
#include <errno.h>
#include <linux/sched.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "algorithms.h"

typedef struct _PSMGR_THREAD {
	pid_t _pid;
	pid_t _ppid;
	char* _fullThreadName;
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
#define STACK_ALLOC() (void*)KMALLOC(char, STACK_SIZE)
#define STACK_FREE(var) KFREE(var)

// internal API

static void i_psmgrCancelRoutine(int signo) {
	DPRINTF(TEXT("signo=%d ppid=%u pid=%u"), signo, getppid(), getpid());
	PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)doublylinkedlistFind(g_psmgrCfg._threadList, getpid());
	DPRINTF(TEXT("threadName=%s p_cancelRoutine=%p"), p_threadCtx->_fullThreadName, p_threadCtx->_p_cancelRoutine);
	p_threadCtx->_p_cancelRoutine(p_threadCtx->_p_arg);
	//release lock from execRoutine
	doublylinkedlistRelease(p_threadCtx);
	//release lock from cancelRoutine
	doublylinkedlistRelease(p_threadCtx);
	doublylinkedlistDel(g_psmgrCfg._threadList, p_threadCtx);
	_exit(1);
}

static KSTATUS i_psmgrExecRoutine(void* p_arg) {
    DPRINTF(TEXT("ENTER ppid=%u pid=%u"), getppid(), getpid());
    PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)p_arg;
    p_threadCtx->_pid = getpid();
    p_threadCtx->_ppid = getppid();
    p_threadCtx = doublylinkedlistAdd(g_psmgrCfg._threadList, getpid(), p_threadCtx, sizeof(PSMGR_THREAD));
	if(signal(SIGFPE, i_psmgrCancelRoutine) == SIG_ERR)
		return -1;
	int ret = p_threadCtx->_p_execRoutine(p_threadCtx->_p_arg);
	signal(SIGFPE, SIG_DFL);
	doublylinkedlistDel(g_psmgrCfg._threadList, p_threadCtx);
    DPRINTF(TEXT("RET ppid=%u pid=%u"), getppid(), getpid());
	return ret;
}

static void i_psmgrDumpList(void) {
	char buff[4096];
	PDOUBLYLINKEDLIST_QUERY pquery;
	const size_t current_size = sizeof(buff);
	size_t required_size;

    DPRINTF(TEXT("DUMP_START"));
	pquery = (PDOUBLYLINKEDLIST_QUERY)buff;
	required_size = current_size;
	if(!doublylinkedlistQuery(g_psmgrCfg._threadList, pquery, &required_size)) {
		DPRINTF(TEXT("Too much entries!"));
	}
	DPRINTF(TEXT("curr_size=%lu required_size=%lu"), current_size, required_size);
	while(!doublylinkedlistQueryIsEnd(pquery)) {
		DPRINTF(TEXT("Entry key=%lu references=%u isDeleted=%c size=%lu"), pquery->_key, pquery->_references, (pquery->_isDeleted) ? 'Y' : 'N', pquery->_size);
		PPSMGR_THREAD pthread = (PPSMGR_THREAD)pquery->_p_userData;
		DPRINTF(TEXT("Entry_thread pid=%d ppid=%d fullThreadName=%s"), pthread->_pid, pthread->_ppid, pthread->_fullThreadName);
		pquery = doublylinkedlistQueryNext(pquery);
	}
    DPRINTF(TEXT("DUMP_STOP"));
}

// external API

KSTATUS psmgrStart(void) {
	DPRINTF(TEXT("Starting ..."));
	g_psmgrCfg._threadList = doublylinkedlistAlloc();
	if(g_psmgrCfg._threadList == NULL)
		return KSTATUS_UNSUCCESS;
	return KSTATUS_SUCCESS;
}

void psmgrStop(void) {
    i_psmgrDumpList();
	DPRINTF("Clearing up...");
	while(!doublylinkedlistIsEmpty(g_psmgrCfg._threadList)) {
		PPSMGR_THREAD p_threadCtx = (PPSMGR_THREAD)doublylinkedlistGetFirst(g_psmgrCfg._threadList);
		DPRINTF(TEXT("Killing pid=%d ..."), p_threadCtx->_pid);
		kill(p_threadCtx->_pid, SIGFPE);
		doublylinkedlistRelease(p_threadCtx);
		doublylinkedlistFreeDeletedEntries(g_psmgrCfg._threadList);
	}
	doublylinkedlistFree(g_psmgrCfg._threadList);
}

KSTATUS psmgrCreateThread(const char* c_threadName, psmgr_execRoutine p_execRoutine, psmgr_cancelRoutine p_cancelRoutine, void *p_arg) {
	KSTATUS _status = KSTATUS_UNSUCCESS;
	PSMGR_THREAD threadCtx;
	pid_t pid;

    threadCtx._p_arg = p_arg;
    threadCtx._p_execRoutine = p_execRoutine;
    threadCtx._p_cancelRoutine = p_cancelRoutine;
    threadCtx._fullThreadName = (char*)c_threadName;
    threadCtx._p_stack = STACK_ALLOC();
    if(threadCtx._p_stack == NULL) {
        return KSTATUS_OUT_OF_MEMORY;
    }
    //CLONE_VM - common memory space for parent and child process
    //CLONE_FS - common file system information (This includes the root of the file system, the current working directory, and the umask)
    //CLONE_FILES - common file descriptor table
    pid = clone(i_psmgrExecRoutine, threadCtx._p_stack + STACK_SIZE-1, CLONE_FILES | CLONE_VM | CLONE_FS, &threadCtx);
    if(pid != -1) {
        DPRINTF(TEXT("threadName=%s pid=%u ppid=%u p_execRoutine=%p p_cancelRoutine=%p"), c_threadName, pid, getpid(), p_execRoutine, p_cancelRoutine);
        if(waitpid(pid, NULL, __WALL) == -1) {
            DPRINTF(TEXT("errno %u"), errno);
        } else _status = KSTATUS_SUCCESS;
    }
	STACK_FREE(threadCtx._p_stack);
    return _status;
}

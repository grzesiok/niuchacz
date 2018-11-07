#include <unistd.h>
#include <stdio.h>
#include "cmd_manager.h"
#include "algorithms.h"

static const char* cgCreateSchemaCmdList =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, version integer, pexec integer, pcreate integer, pdestroy integer"
		");";
static const char* cgInsertCommand =
		"insert into cmdmgr_cmdlist(code, description, version, pexec, pcreate, pdestroy)"
		"values (?, ?, ?, ?, ?, ?);";

typedef struct {
    queue_t *_pjobQueueShortOps;
    queue_t *_pjobQueueLongOps;
} CMD_MANAGER, *PCMD_MANAGER;

CMD_MANAGER gCmdManager;

const char *cg_CmdMgr_ShortOps_ShortName = "niuch_cmdshrt";
const char *cg_CmdMgr_LongOps_ShortName = "niuch_cmdlong";
const char *cg_CmdMgr_ShortOps = "Short Operation Queue";
const char *cg_CmdMgr_LongOps = "Long Operation Queue";
const char *cg_CmdMgr_ShortOps_FullName = "Command Manager - Short Operation Queue";
const char *cg_CmdMgr_LongOps_FullName = "Command Manager - Long Operation Queue";

/* Internal API */

int i_cmdmgrFindPtr_GetVal(void *retVal, sqlite3_stmt* stmt) {
    void* ptr = (void*)sqlite3_column_int64(stmt, 0);
    memcpy(retVal, (void*)&ptr, sizeof(void*));
    return 0;
}

static void* i_cmdmgrFindPtr(const char* cmd, const char* column_name) {
    KSTATUS _status;
    void* ptr = NULL;
    char formatted_query[255];

    sprintf(formatted_query, "select %s from cmdmgr_cmdlist where code = ?;", column_name);
    _status = dbExecQuery(svcKernelGetDb(), formatted_query, 1, i_cmdmgrFindPtr_GetVal, &ptr, DB_BIND_TEXT, cmd);
    if(!KSUCCESS(_status))
        ptr = NULL;
    return ptr;
}

static PJOB_CREATE i_cmdmgrFindCreate(const char* cmd) {
    return (PJOB_CREATE)i_cmdmgrFindPtr(cmd, "pcreate");
}

static PJOB_DESTROY i_cmdmgrFindDestroy(const char* cmd) {
    return (PJOB_DESTROY)i_cmdmgrFindPtr(cmd, "pdestroy");
}

static PJOB_EXEC i_cmdmgrFindExec(const char* cmd) {
    return (PJOB_EXEC)i_cmdmgrFindPtr(cmd, "pexec");
}

int i_cmdmgrDestroySingleCommand(void *NotUsed, sqlite3_stmt* stmt) {
    if(sqlite3_column_count(stmt) == 0) {
        SYSLOG(LOG_ERR, "argc = 0");
        return 0;
    }
    SYSLOG(LOG_INFO, "Destroying %s", sqlite3_column_text(stmt, 0));
    PJOB_DESTROY pdestroy = i_cmdmgrFindDestroy((const char*)sqlite3_column_text(stmt, 0));
    if(pdestroy == NULL) {
        return 0;
    }
    pdestroy();
    return 0;
}

KSTATUS i_cmdmgrJobExec(PJOB pjob) {
    int ret;
    PJOB_EXEC pexec;

    if(!svcKernelIsRunning()) {
        return KSTATUS_SVC_IS_STOPPING;
    }
    pexec = i_cmdmgrFindExec(pjob->_cmd);
    if(pexec == NULL)
        return KSTATUS_CMDMGR_COMMAND_NOT_FOUND;
    ret = pexec(pjob->_ts, pjob->_data, pjob->_dataSize);
    if(ret != 0)
        //job should be rescheduled again
        //or job should be freed if rescheduled is not an option
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

KSTATUS i_cmdmgrExecutor(void* arg) {
    KSTATUS _status = KSTATUS_SUCCESS;
    char buffer[100000];
    PJOB pjob = (PJOB)buffer;
    queue_t* pqueue = (queue_t*)arg;
    int ret;
    static struct timespec time_to_wait = {0, 0};
    const char* queueName = (arg == (void*)gCmdManager._pjobQueueShortOps) ? cg_CmdMgr_ShortOps : cg_CmdMgr_LongOps;

    SYSLOG(LOG_INFO, "[CMDMGR][%s] Starting Job Executor", queueName);
    if(!queue_consumer_new(pqueue)) {
        _status = KSTATUS_UNSUCCESS;
        //TODO: Restrt cmdmgrExecutor and queue on the fly
        goto __cleanup;
    }
    while(svcKernelIsRunning()) {
        timerGetRealCurrentTimestamp(&time_to_wait);
        time_to_wait.tv_sec += 1;
        ret = queue_read(pqueue, pjob, &time_to_wait);
        if(ret > 0) {
            i_cmdmgrJobExec(pjob);
        } else if(ret == QUEUE_RET_ERROR) {
            SYSLOG(LOG_ERR, "[CMDMGR][%s] Error during dequeue job", queueName);
        }
    }
    queue_consumer_free(pqueue);
__cleanup:
    SYSLOG(LOG_INFO, "[CMDMGR][%s] Stopping Job Executor", queueName);
    return _status;
}

/* External API */

KSTATUS cmdmgrStart(void) {
    KSTATUS _status;
    SYSLOG(LOG_INFO, "[CMDMGR] Starting...");
    _status = dbExec(svcKernelGetDb(), cgCreateSchemaCmdList, 0);
    if(!KSUCCESS(_status))
        return _status;
    gCmdManager._pjobQueueShortOps = queue_create(1024*1024);//1MB
    if(gCmdManager._pjobQueueShortOps == NULL)
        return KSTATUS_UNSUCCESS;
    gCmdManager._pjobQueueLongOps = queue_create(1024*1024/*1024*/);//1GB (should be 1MB at the beginning) TODO: dynamically increase queue_size
    if(gCmdManager._pjobQueueLongOps == NULL)
        return KSTATUS_UNSUCCESS;
    _status = psmgrCreateThread(cg_CmdMgr_ShortOps_ShortName, cg_CmdMgr_ShortOps_FullName, PSMGR_THREAD_KERNEL, i_cmdmgrExecutor, NULL, gCmdManager._pjobQueueShortOps);
    if(!KSUCCESS(_status))
        return _status;
    _status = psmgrCreateThread(cg_CmdMgr_LongOps_ShortName, cg_CmdMgr_LongOps_FullName, PSMGR_THREAD_KERNEL, i_cmdmgrExecutor, NULL, gCmdManager._pjobQueueLongOps);
    return _status;
}

void cmdmgrStop(void) {
    SYSLOG(LOG_INFO, "[CMDMGR] Stopping...");
    queue_destroy(gCmdManager._pjobQueueShortOps);
    queue_destroy(gCmdManager._pjobQueueLongOps);
    SYSLOG(LOG_INFO, "[CMDMGR] Cleaning up commands...");
    dbExecQuery(svcKernelGetDb(), "select code from cmdmgr_cmdlist", 0, i_cmdmgrDestroySingleCommand, NULL);
}

KSTATUS cmdmgrAddCommand(const char* cmd, const char* description, PJOB_EXEC pexec, PJOB_CREATE pcreate, PJOB_DESTROY pdestroy, int version) {
    int ret;
    KSTATUS _status = KSTATUS_UNSUCCESS;

    ret = pcreate();
    if(ret != 0)
        goto __cleanup;
    _status = dbExec(svcKernelGetDb(), cgInsertCommand, 6,
                     DB_BIND_TEXT, cmd,
                     DB_BIND_TEXT, description, 
                     DB_BIND_INT, version,
                     DB_BIND_INT64, (sqlite_int64)pexec,
                     DB_BIND_INT64, (sqlite_int64)pcreate,
                     DB_BIND_INT64, (sqlite_int64)pdestroy);
__cleanup:
    if(!KSUCCESS(_status)) {
        pdestroy();
        SYSLOG(LOG_ERR, "Failed to create command=%s: %d", cmd, ret);
        return _status;
    }
    SYSLOG(LOG_INFO, "Command %s(%p, %p, %p) added successfully", cmd, pexec, pcreate, pdestroy);
    return KSTATUS_SUCCESS;
}

KSTATUS cmdmgrJobPrepare(const char* cmd, void* pdata, size_t dataSize, struct timeval ts, PJOB* pjob) {
    PJOB pjob2;
    int cmdLength = strlen(cmd);
    DPRINTF("Allocate %zuB memory for command %s", dataSize, cmd);

    pjob2 = MALLOC2(JOB, 1, dataSize+cmdLength+1);
    if(pjob2 == NULL)
        return KSTATUS_OUT_OF_MEMORY;
    pjob2->_cmd = memoryPtrMove(pjob2, sizeof(JOB));
    memcpy(pjob2->_cmd, cmd, cmdLength);
    pjob2->_cmd[cmdLength] = '\0';
    pjob2->_ts = ts;
    pjob2->_data = memoryPtrMove(pjob2->_cmd, cmdLength+1);
    memcpy(pjob2->_data, pdata, dataSize);
    pjob2->_dataSize = dataSize;
    *pjob = pjob2;
    return KSTATUS_SUCCESS;
}

KSTATUS cmdmgrJobExec(PJOB pjob, JobMode mode, JobQueueType queueType) {
    KSTATUS _status = KSTATUS_UNSUCCESS;
    int ret = 0;
    switch(mode) {
    //if mode == JobModeAsynchronous then function should back immediatelly and schedule job to future
    case JobModeAsynchronous:
        switch(queueType) {
        case JobQueueTypeNone:
            return KSTATUS_UNSUCCESS;
        case JobQueueTypeShortOps:
            if(queue_producer_new(gCmdManager._pjobQueueShortOps)) {
                ret = queue_write(gCmdManager._pjobQueueShortOps, pjob, pjob->_dataSize+sizeof(JOB), NULL);
                queue_producer_free(gCmdManager._pjobQueueShortOps);
            }
            break;
        case JobQueueTypeLongOps:
            if(queue_producer_new(gCmdManager._pjobQueueLongOps)) {
                ret = queue_write(gCmdManager._pjobQueueLongOps, pjob, pjob->_dataSize+sizeof(JOB), NULL);
                queue_producer_free(gCmdManager._pjobQueueLongOps);
            }
            break;
        }
        if(ret == pjob->_dataSize+sizeof(JOB))
            _status = KSTATUS_SUCCESS;
        break;
    //if mode == JobModeSynchronous then function should wait until execution is done
    case JobModeSynchronous:
        _status = i_cmdmgrJobExec(pjob);
        FREE(pjob);
        break;
    }
    return _status;
}

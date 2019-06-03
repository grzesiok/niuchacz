#include <unistd.h>
#include <stdio.h>
#include "cmd_manager.h"
#include "algorithms.h"
#include "flags.h"

static const char* cgCreateSchemaCmdList =
		"create table if not exists cmdmgr_cmdlist ("
		"code text, description text, version integer, pjobdef integer, pexec integer, pcreate integer, pdestroy integer"
		");";
static const char* cgInsertCommand =
		"insert into cmdmgr_cmdlist(code, description, version, pjobdef, pexec, pcreate, pdestroy)"
		"values (?, ?, ?, ?, ?, ?, ?);";

typedef struct {
    const char* _queueName;
    const char* _queueShortName;
    const char* _queueFullName;
    queue_t *_pjobQueue;
    stats_entry_t _statsEntry_EntriesCurrent;
    stats_entry_t _statsEntry_EntriesMax;
    stats_entry_t _statsEntry_MemUsageCurrent;
    stats_entry_t _statsEntry_MemUsageMax;
    stats_entry_t _statsEntry_MemSizeCurrent;
    stats_entry_t _statsEntry_MemSizeMax;
} cmd_manager_queue_t;
#define CMD_MANAGER_QUEUE_SHORTOPS 0
#define CMD_MANAGER_QUEUE_LONGOPS 1
#define CMD_MANAGER_QUEUE_MAX 2

typedef struct {
    cmd_manager_queue_t _queues[CMD_MANAGER_QUEUE_MAX];
} CMD_MANAGER, *PCMD_MANAGER;

CMD_MANAGER gCmdManager;

const char* gc_statsKey_KernelShortOpsEntriesCurrent = "ShortOps Entries Current";
const char* gc_statsKey_KernelShortOpsEntriesMax = "ShortOps Entries Max";
const char* gc_statsKey_KernelShortOpsMemUsageCurrent = "ShortOps Memory Usage Current";
const char* gc_statsKey_KernelShortOpsMemUsageMax = "ShortOps Memory Usage Max";
const char* gc_statsKey_KernelShortOpsMemSizeCurrent = "ShortOps Memory Size Current";
const char* gc_statsKey_KernelShortOpsMemSizeMax = "ShortOps Memory Size Max";
const char* gc_statsKey_KernelLongOpsEntriesCurrent = "LongOps Entries Current";
const char* gc_statsKey_KernelLongOpsEntriesMax = "LongOps Entries Max";
const char* gc_statsKey_KernelLongOpsMemUsageCurrent = "LongOps Memory Usage Current";
const char* gc_statsKey_KernelLongOpsMemUsageMax = "LongOps Memory Usage Max";
const char* gc_statsKey_KernelLongOpsMemSizeCurrent = "LongOps Memory Size Current";
const char* gc_statsKey_KernelLongOpsMemSizeMax = "LongOps Memory Size Max";

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

void i_cmdmgrJobStructInitialize(PJOB pjob, const char* cmd) {
    pjob->_cmd = memoryPtrMove(pjob, sizeof(JOB));
    pjob->_data = memoryPtrMove(pjob->_cmd, strlen(cmd)+1);
}

void i_cmdmgrJobStructFormat(PJOB pjob) {
    pjob->_cmd = memoryPtrMove(pjob, sizeof(JOB));
    pjob->_data = memoryPtrMove(pjob->_cmd, strlen(pjob->_cmd)+1);
}

KSTATUS i_cmdmgrJobExec(PJOB pjob) {
    int ret;
    PJOB_EXEC pexec;

    if(!svcKernelIsRunning()) {
        return KSTATUS_SVC_IS_STOPPING;
    }
    pexec = i_cmdmgrFindExec(pjob->_cmd);
    if(pexec == NULL) {
        SYSLOG(LOG_ERR, "[CMDMGR] Command not found %s", pjob->_cmd);
        return KSTATUS_CMDMGR_COMMAND_NOT_FOUND;
    }
    ret = pexec(pjob->_ts, pjob->_data, pjob->_dataSize);
    if(ret != 0)
        //job should be rescheduled again
        //or job should be freed if rescheduled is not an option
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

KSTATUS i_cmdmgrExecutor(void* arg) {
    KSTATUS _status = KSTATUS_SUCCESS;
    void* buffer;
    PJOB pjob;
    cmd_manager_queue_t* pThreadCtx = (cmd_manager_queue_t*)arg;
    int ret;
    static struct timespec time_to_wait = {0, 0};

    SYSLOG(LOG_INFO, "[CMDMGR][%s] Starting Job Executor", pThreadCtx->_queueName);
    buffer = MALLOC(char, 1024*1024); //Allocating 1MB per each executor
    if(buffer == NULL) {
        SYSLOG(LOG_ERR, "[CMDMGR][%s] Error during allocating local memory", pThreadCtx->_queueName);
        _status = KSTATUS_UNSUCCESS;
        //TODO: Restrt cmdmgrExecutor and queue on the fly
        goto __cleanup;
    }
    pjob = (PJOB)buffer;
    if(!queue_consumer_new(pThreadCtx->_pjobQueue)) {
        SYSLOG(LOG_ERR, "[CMDMGR][%s] Error during attaching queue", pThreadCtx->_queueName);
        _status = KSTATUS_UNSUCCESS;
        //TODO: Restrt cmdmgrExecutor and queue on the fly
        goto __cleanup_free_tls;
    }
    while(svcKernelIsRunning()) {
        timerGetRealCurrentTimestamp(&time_to_wait);
        time_to_wait.tv_sec += 1;
        ret = queue_read(pThreadCtx->_pjobQueue, pjob, &time_to_wait);
        if(ret > 0) {
            /* Reformat structure after pulling it from queue */
            i_cmdmgrJobStructFormat(pjob);
            if(!KSUCCESS(i_cmdmgrJobExec(pjob))) {
                SYSLOG(LOG_ERR, "[CMDMGR][%s] Error during executing job(%s, %d)", pThreadCtx->_queueName, pjob->_cmd, ret);
            }
        } else if(ret == QUEUE_RET_ERROR) {
            SYSLOG(LOG_ERR, "[CMDMGR][%s] Error during dequeue job", pThreadCtx->_queueName);
        }
        statsUpdate(&pThreadCtx->_statsEntry_EntriesCurrent, pThreadCtx->_pjobQueue->_stats_EntriesCurrent);
        statsUpdate(&pThreadCtx->_statsEntry_EntriesMax, pThreadCtx->_pjobQueue->_stats_EntriesMax);
        statsUpdate(&pThreadCtx->_statsEntry_MemUsageCurrent, pThreadCtx->_pjobQueue->_stats_MemUsageCurrent);
        statsUpdate(&pThreadCtx->_statsEntry_MemUsageMax, pThreadCtx->_pjobQueue->_stats_MemUsageMax);
        statsUpdate(&pThreadCtx->_statsEntry_MemSizeCurrent, pThreadCtx->_pjobQueue->_stats_MemSizeCurrent);
        statsUpdate(&pThreadCtx->_statsEntry_MemSizeMax, pThreadCtx->_pjobQueue->_stats_MemSizeMax);
    }
    SYSLOG(LOG_INFO, "[CMDMGR][%s] Detaching queue", pThreadCtx->_queueName);
    queue_consumer_free(pThreadCtx->_pjobQueue);
__cleanup_free_tls:
    FREE(buffer);
__cleanup:
    SYSLOG(LOG_INFO, "[CMDMGR][%s] Stopping Job Executor", pThreadCtx->_queueName);
    return _status;
}

/* External API */

KSTATUS cmdmgrStart(void) {
    KSTATUS _status;
    cmd_manager_queue_t* pqueue;
    SYSLOG(LOG_INFO, "[CMDMGR] Starting...");

    gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._queueName = "Short Operation Queue";
    gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._queueShortName = "niuch_cmdshrt";
    gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._queueFullName = "Command Manager - Short Operation Queue";
    gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._pjobQueue = queue_create(1024*1024*1024);
    if(gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._pjobQueue == NULL)
        return KSTATUS_UNSUCCESS;
    stats_bulk_init_t s_stats_shortops[] = {
        {gc_statsKey_KernelShortOpsEntriesCurrent, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._statsEntry_EntriesCurrent},
        {gc_statsKey_KernelShortOpsEntriesMax, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._statsEntry_EntriesMax},
        {gc_statsKey_KernelShortOpsMemUsageCurrent, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._statsEntry_MemUsageCurrent},
        {gc_statsKey_KernelShortOpsMemUsageMax, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._statsEntry_MemUsageMax},
        {gc_statsKey_KernelShortOpsMemSizeCurrent, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._statsEntry_MemSizeCurrent},
        {gc_statsKey_KernelShortOpsMemSizeMax, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._statsEntry_MemSizeMax}};
    _status = statsAllocBulk(svcKernelGetStatsList(), s_stats_shortops, sizeof(s_stats_shortops)/sizeof(s_stats_shortops[0]));
    if(!KSUCCESS(_status))
        return _status;
    gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._queueName = "Long Operation Queue";
    gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._queueShortName = "niuch_cmdlong";
    gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._queueFullName = "Command Manager - Long Operation Queue";
    gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._pjobQueue = queue_create(1024*1024);
    if(gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._pjobQueue == NULL)
        return KSTATUS_UNSUCCESS;
    stats_bulk_init_t s_stats_longops[] = {
        {gc_statsKey_KernelLongOpsEntriesCurrent, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._statsEntry_EntriesCurrent},
        {gc_statsKey_KernelLongOpsEntriesMax, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._statsEntry_EntriesMax},
        {gc_statsKey_KernelLongOpsMemUsageCurrent, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._statsEntry_MemUsageCurrent},
        {gc_statsKey_KernelLongOpsMemUsageMax, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._statsEntry_MemUsageMax},
        {gc_statsKey_KernelLongOpsMemSizeCurrent, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._statsEntry_MemSizeCurrent},
        {gc_statsKey_KernelLongOpsMemSizeMax, STATS_FLAGS_TYPE_LAST, &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._statsEntry_MemSizeMax}};
    _status = statsAllocBulk(svcKernelGetStatsList(), s_stats_longops, sizeof(s_stats_longops)/sizeof(s_stats_longops[0]));
    if(!KSUCCESS(_status))
        return _status;
    _status = dbExec(svcKernelGetDb(), cgCreateSchemaCmdList, 0);
    if(!KSUCCESS(_status))
        return _status;
    pqueue = &gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS];
    _status = psmgrCreateThread(pqueue->_queueShortName, pqueue->_queueFullName, PSMGR_THREAD_KERNEL, i_cmdmgrExecutor, NULL, pqueue);
    if(!KSUCCESS(_status))
        return _status;
    pqueue = &gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS];
    _status = psmgrCreateThread(pqueue->_queueShortName, pqueue->_queueFullName, PSMGR_THREAD_KERNEL, i_cmdmgrExecutor, NULL, pqueue);
    return _status;
}

void cmdmgrStop(void) {
    SYSLOG(LOG_INFO, "[CMDMGR] Stopping %s...", gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._queueName);
    queue_destroy(gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._pjobQueue);
    SYSLOG(LOG_INFO, "[CMDMGR] Stopping %s...", gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._queueName);
    queue_destroy(gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._pjobQueue);
    SYSLOG(LOG_INFO, "[CMDMGR] Cleaning up commands...");
    dbExecQuery(svcKernelGetDb(), "select code from cmdmgr_cmdlist", 0, i_cmdmgrDestroySingleCommand, NULL);
    SYSLOG(LOG_INFO, "[CMDMGR] Cleaning up...");
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
                     DB_BIND_INT64, (sqlite_int64)NULL,/* TODO: job definition not implemented */
                     DB_BIND_INT64, (sqlite_int64)pexec,
                     DB_BIND_INT64, (sqlite_int64)pcreate,
                     DB_BIND_INT64, (sqlite_int64)pdestroy);
__cleanup:
    if(!KSUCCESS(_status)) {
        pdestroy();
        SYSLOG(LOG_ERR, "[CMDMGR] Failed to create command=%s: %d", cmd, ret);
        return _status;
    }
    SYSLOG(LOG_INFO, "[CMDMGR] Command %s(%p, %p, %p) added successfully", cmd, pexec, pcreate, pdestroy);
    return KSTATUS_SUCCESS;
}

KSTATUS cmdmgrJobPrepare(const char* cmd, void* pdata, size_t dataSize, struct timeval ts, uint32_t flags, PJOB* pjob) {
    PJOB pjob2;
    
    pjob2 = MALLOC2(JOB, 1, dataSize+strlen(cmd)+1);
    if(pjob2 == NULL)
        return KSTATUS_OUT_OF_MEMORY;
    i_cmdmgrJobStructInitialize(pjob2, cmd);
    strcpy(pjob2->_cmd, cmd);
    pjob2->_ts = ts;
    pjob2->_flags = flags;
    memcpy(pjob2->_data, pdata, dataSize);
    pjob2->_dataSize = dataSize;
    *pjob = pjob2;
    return KSTATUS_SUCCESS;
}

void cmdmgrJobCleanup(PJOB pjob) {
    FREE(pjob);
}

KSTATUS cmdmgrJobExec(PJOB pjob, JobMode mode, JobQueueType queueType) {
    KSTATUS _status = KSTATUS_UNSUCCESS;
    int ret = 0;
    size_t queueEntrySize;
    queue_t* pqueue;

    switch(mode) {
    //if mode == JobModeAsynchronous then function should back immediatelly and schedule job to future
    case JobModeAsynchronous:
        queueEntrySize = pjob->_dataSize+strlen(pjob->_cmd)+1+sizeof(JOB);
        switch(queueType) {
        default:
        case JobQueueTypeNone:
            return KSTATUS_UNSUCCESS;
        case JobQueueTypeShortOps:
            pqueue = gCmdManager._queues[CMD_MANAGER_QUEUE_SHORTOPS]._pjobQueue;
            break;
        case JobQueueTypeLongOps:
            pqueue = gCmdManager._queues[CMD_MANAGER_QUEUE_LONGOPS]._pjobQueue;
            break;
        }
        if(queue_producer_new(pqueue)) {
            ret = queue_write(pqueue, pjob, queueEntrySize, NULL);
            queue_producer_free(pqueue);
            if(ret > 0 && (size_t)ret == queueEntrySize) {
                _status = KSTATUS_SUCCESS;
            }
        }
        cmdmgrJobCleanup(pjob);
        break;
    //if mode == JobModeSynchronous then function should wait until execution is done
    case JobModeSynchronous:
        _status = i_cmdmgrJobExec(pjob);
        cmdmgrJobCleanup(pjob);
        break;
    }
    return _status;
}

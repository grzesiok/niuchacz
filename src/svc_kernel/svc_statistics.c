#include "svc_kernel/svc_statistics.h"
#include "svc_kernel/svc_lock.h"
#include "algorithms.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

typedef struct {
    PDOUBLYLINKEDLIST _statsList;
} stats_mgr_t;

stats_mgr_t g_statsMgr;


//internal API
int i_statsmgrDumpSingleListCallback(void *data, const unsigned char *key, uint32_t key_len, void *value) {
    stats_entry_t *p_entry = (stats_entry_t*)value;
    SYSLOG(LOG_INFO, "[STATSMGR] Entry flags=%08x value=%llu name=%.*s", p_entry->_flags, p_entry->_value, key_len, key);
    return 0;
}

KSTATUS i_statsmgrDumpSingleList(stats_list_t* p_stats) {
    SYSLOG(LOG_INFO, "[STATSMGR] List name=%s entries=%"PRIu64"", p_stats->_name, art_size(&p_stats->_tree));
    if(art_iter(&p_stats->_tree, i_statsmgrDumpSingleListCallback, NULL) != 0)
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

//external API
KSTATUS statsmgrStart(void) {
    SYSLOG(LOG_INFO, "[STATSMGR] Starting ...");
    g_statsMgr._statsList = doublylinkedlistAlloc();
    if(g_statsMgr._statsList == NULL)
        return KSTATUS_UNSUCCESS;
    return KSTATUS_SUCCESS;
}

void statsmgrStop(void) {
    SYSLOG(LOG_INFO, "[STATSMGR] Stopping ...");
    SYSLOG(LOG_INFO, "[STATSMGR] Cleaning up...");
    doublylinkedlistFreeDeletedEntries(g_statsMgr._statsList);
    doublylinkedlistFree(g_statsMgr._statsList);
}

KSTATUS statsmgrDump(void) {
    char buff[4096];
    PDOUBLYLINKEDLIST_QUERY pquery;
    const size_t current_size = sizeof(buff);
    size_t required_size;

    SYSLOG(LOG_INFO, "[STATSMGR] DUMP START ...");
    pquery = (PDOUBLYLINKEDLIST_QUERY)buff;
    required_size = current_size;
    if(!doublylinkedlistQuery(g_statsMgr._statsList, pquery, &required_size)) {
        SYSLOG(LOG_DEBUG, "[STATSMGR] Too much entries!");
        return KSTATUS_UNSUCCESS;
    }
    SYSLOG(LOG_INFO, "[STATSMGR] curr_size=%lu required_size=%lu", current_size, required_size);
    while(!doublylinkedlistQueryIsEnd(pquery)) {
        SYSLOG(LOG_INFO, "[STATSMGR] Entry key=%lu references=%u isDeleted=%c size=%lu", pquery->_key, pquery->_references, (pquery->_isDeleted) ? 'Y' : 'N', pquery->_size);
        i_statsmgrDumpSingleList((stats_list_t*)pquery->_p_userData);
        pquery = doublylinkedlistQueryNext(pquery);
    }
    SYSLOG(LOG_INFO, "[STATSMGR] DUMP STOP ...");
    return KSTATUS_SUCCESS;
}

stats_list_t* statsCreate(const char* listName) {
    stats_list_t* p_tmp_stats;
    stats_list_t* p_stats;
    uint32_t additional_space;

    additional_space = strlen(listName)+1;
    p_tmp_stats = MALLOC2(stats_list_t, 1, additional_space);
    if(p_tmp_stats == NULL)
        return NULL;
    memset(p_tmp_stats, 0, sizeof(stats_list_t)+additional_space);
    p_tmp_stats->_name = memoryPtrMove(p_tmp_stats, sizeof(stats_list_t));
    strcpy(p_tmp_stats->_name, listName);
    if(art_tree_init(&p_tmp_stats->_tree) != 0) {
        FREE(p_tmp_stats);
        return NULL;
    }
    SYSLOG(LOG_INFO, "statsCreate(%s) adding To List ...", p_tmp_stats->_name);
    p_stats = doublylinkedlistAdd(g_statsMgr._statsList, *((uint64_t*)listName), p_tmp_stats, sizeof(stats_list_t)+additional_space);
    p_stats->_name = memoryPtrMove(p_stats, sizeof(stats_list_t));
    FREE(p_tmp_stats);
    SYSLOG(LOG_INFO, "statsCreate(%s) added ...", p_stats->_name);
    return p_stats;
}

void statsDestroy(stats_list_t* p_stats) {
    SYSLOG(LOG_INFO, "statsDestroy(%s)", p_stats->_name);
    doublylinkedlistDel(g_statsMgr._statsList, p_stats);
    art_tree_destroy(&p_stats->_tree);
    doublylinkedlistRelease(p_stats);
}

KSTATUS statsAlloc(stats_list_t* p_stats, const char* statsName, int flags, stats_entry_t* p_entry) {
    SYSLOG(LOG_INFO, "statsAlloc(%s, %s[%zd], flags=%d, ptr=%p)", p_stats->_name, statsName, strlen(statsName), flags, p_entry);
    p_entry->_flags = flags;
    p_entry->_value = 0;
    if(art_insert(&p_stats->_tree, (const unsigned char*)statsName, strlen(statsName), p_entry) == p_entry) {
        return KSTATUS_UNSUCCESS;
    }
    return KSTATUS_SUCCESS;
}

KSTATUS statsAllocBulk(stats_list_t* p_stats, stats_bulk_init_t* p_stats_array, int stats_num) {
    KSTATUS _status = KSTATUS_SUCCESS;
    int i;

    for(i = 0;i < stats_num;i++) {
        _status = statsAlloc(p_stats, p_stats_array[i]._statsEntry_key, p_stats_array[i]._statsEntry_flags, p_stats_array[i]._statsEntry_ptr);
        if(!KSUCCESS(_status)) {
            SYSLOG(LOG_ERR, "statsAllocBulk: Error during allocation StatsKey(%s)!", p_stats_array[i]._statsEntry_key);
            return _status;
        }
    }
    return _status;
}

void statsFree(stats_list_t* p_stats, const char* statsName) {
    SYSLOG(LOG_INFO, "statsFree(%s, %s)", p_stats->_name, statsName);
    art_delete(&p_stats->_tree, (const unsigned char*)statsName, strlen(statsName));
}

stats_entry_t* statsFind(stats_list_t* p_stats, const char* statsName) {
    return art_search(&p_stats->_tree, (const unsigned char*)statsName, strlen(statsName));
}

unsigned long long statsUpdate(stats_entry_t *p_entry, unsigned long long value) {
    switch(p_entry->_flags) {
        case STATS_FLAGS_TYPE_SUM:
            return __atomic_add_fetch(&p_entry->_value, value, __ATOMIC_RELEASE);
        case STATS_FLAGS_TYPE_LAST:
            __atomic_store(&p_entry->_value, &value, __ATOMIC_RELEASE);
            return value;
    }
    return 0;
}

unsigned long long statsGetValue(stats_entry_t* p_entry) {
    return p_entry->_value;
}

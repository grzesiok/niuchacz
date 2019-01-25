#include "svc_kernel/svc_statistics.h"
#include "svc_kernel/svc_lock.h"
#include "algorithms.h"

typedef struct {
    PDOUBLYLINKEDLIST _statsList;
} stats_mgr_t;

stats_mgr_t g_statsMgr;


//internal API

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

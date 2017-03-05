#include "svc_statistics.h"
#include "svc_lock.h"

#define STATS_ENTRY_FLAGS_FREE 0
#define STATS_ENTRY_FLAGS_USED 1

typedef struct _STATS_ENTRY {
	unsigned char _flags;
	char _statsName[STATS_ENTRY_NAME_MAXSIZE];
	int _type;
	unsigned long long _value;
} STATS_ENTRY, *PSTATS_ENTRY;

#define STATS_BLOCK_MAX_ENTRIES 32

typedef struct _STATS_LEAFBLOCK {
	STATS_ENTRY _p_entries[STATS_BLOCK_MAX_ENTRIES];
	struct _STATS_LEAFBLOCK *_p_nextBlock;
} STATS_LEAFBLOCK, *PSTATS_LEAFBLOCK;

typedef struct _STATS_ROOTBLOCK {
	LOCK_VAR;
	STATS_LEAFBLOCK _leafBlock;
} STATS_ROOTBLOCK, *PSTATS_ROOTBLOCK;

STATS_ROOTBLOCK g_StatsRootBlock;

//internal API
static PSTATS_ENTRY i_statsFindEntry(PSTATS_LEAFBLOCK p_leafBlock, unsigned char expectedFlags, unsigned char desiredFlags) {
	int i;
	for(i = 0;i < STATS_BLOCK_MAX_ENTRIES;i++) {
		if(p_leafBlock->_p_entries[i]._flags != expectedFlags) {
			continue;
		}
		if(!__atomic_compare_exchange(&p_leafBlock->_p_entries[i]._flags, &expectedFlags, &desiredFlags, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
		{
			continue;
		}
		return &p_leafBlock->_p_entries[i];
	}
	return NULL;
}

//external API
KSTATUS statsStart(void) {
	LOCK_INIT(&g_StatsRootBlock, STATS_ROOTBLOCK, "STATS_ROOTBLOCK");
	return KSTATUS_SUCCESS;
}

void statsStop(void) {
	//
}

KSTATUS statsAlloc(const char* statsName, int type, stats_key *p_key) {
	DPRINTF("statsAlloc(%s[%d], %d, %016x)\n", statsName, strlen(statsName), type, p_key);
	if(strlen(statsName) >= STATS_ENTRY_NAME_MAXSIZE)
		return KSTATUS_UNSUCCESS;
	stats_key statsKey = statsFind(statsName);
	if(statsKey != NULL){
		*p_key = statsKey;
		return KSTATUS_SUCCESS;
	}
	KSTATUS _status = KSTATUS_UNSUCCESS;
	LOCK(&g_StatsRootBlock, STATS_ROOTBLOCK);
	PSTATS_LEAFBLOCK p_leafBlock = &g_StatsRootBlock._leafBlock;
	while(p_leafBlock != NULL) {
		PSTATS_ENTRY p_entry = i_statsFindEntry(p_leafBlock, STATS_ENTRY_FLAGS_FREE, STATS_ENTRY_FLAGS_USED);
		if(p_entry != NULL) {
			strncpy(p_entry->_statsName, statsName, STATS_ENTRY_NAME_MAXSIZE);
			p_entry->_type = type;
			printf("%s -> %d\n", p_entry->_statsName, p_entry->_type);
			p_entry->_value = 0;
			statsKey = p_entry;
			_status = KSTATUS_SUCCESS;
			break;
		}
		if(p_leafBlock->_p_nextBlock == NULL) {
			p_leafBlock->_p_nextBlock = MALLOC(STATS_LEAFBLOCK, 1);
			memset(p_leafBlock->_p_nextBlock, 0, sizeof(STATS_LEAFBLOCK));
		}
		p_leafBlock = p_leafBlock->_p_nextBlock;
	}
	UNLOCK(&g_StatsRootBlock, STATS_ROOTBLOCK);
	*p_key = statsKey;
	return _status;
}

void statsFree(stats_key key) {
	DPRINTF("statsFree(key)\n", key);
	PSTATS_ENTRY p_entry = (PSTATS_ENTRY)key;
	p_entry->_flags = STATS_ENTRY_FLAGS_FREE;
}

unsigned long long statsUpdate(stats_key key, unsigned long long value) {
	PSTATS_ENTRY p_entry = (PSTATS_ENTRY)key;
	switch(p_entry->_type) {
	case STATS_TYPE_SUM:
		return __atomic_add_fetch(&p_entry->_value, value, __ATOMIC_RELEASE);
	case STATS_TYPE_LAST:
		__atomic_store(&p_entry->_value, &value, __ATOMIC_RELEASE);
		return value;
	}
	return 0;
}

stats_key statsFind(const char* statsName) {
	stats_key statsKey = NULL;
	LOCK(&g_StatsRootBlock, STATS_ROOTBLOCK);
	PSTATS_LEAFBLOCK p_leafBlock = &g_StatsRootBlock._leafBlock;
	while(p_leafBlock != NULL) {
		PSTATS_ENTRY p_entry = i_statsFindEntry(p_leafBlock, STATS_ENTRY_FLAGS_USED, STATS_ENTRY_FLAGS_USED);
		if(p_entry != NULL && strncmp(p_entry->_statsName, statsName, strlen(p_entry->_statsName)) == 0) {
			statsKey = p_entry;
			break;
		}
		p_leafBlock = p_leafBlock->_p_nextBlock;
	}
	UNLOCK(&g_StatsRootBlock, STATS_ROOTBLOCK);
	return statsKey;
}

unsigned long long statsGetValue(stats_key key) {
	PSTATS_ENTRY p_entry = (PSTATS_ENTRY)key;
	return p_entry->_value;
}

#include "doublylinkedlist.h"
#include "../spinlock/spinlock.h"
#include <stdlib.h>
#include <string.h>

// internal API Entry Header

static void i_doublylinkedlistEntryHeaderInit(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
	pentry->_next = pentry;
	pentry->_prev = pentry;
}

static PDOUBLYLINKEDLIST_ENTRY_HEADER i_doublylinkedlistEntryHeaderNext(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
    return pentry->_next;
}

static PDOUBLYLINKEDLIST_ENTRY_HEADER i_doublylinkedlistEntryHeaderPrev(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
    return pentry->_prev;
}

static void i_doublylinkedlistEntryHeaderAdd(PDOUBLYLINKEDLIST_ENTRY_HEADER plist, PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
	pentry->_next = plist->_next;
    pentry->_prev = plist;
    plist->_next->_prev = pentry;
    plist->_next = pentry;
}

static void i_doublylinkedlistEntryHeaderDel(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
	pentry->_prev->_next = pentry->_next;
	pentry->_next->_prev = pentry->_prev;
	i_doublylinkedlistEntryHeaderInit(pentry);
}

static bool i_doublylinkedlistEntryHeaderIsEmpty(PDOUBLYLINKEDLIST_ENTRY_HEADER plist) {
    if(plist->_next == plist)
        return true;
    return false;
}

static bool i_doublylinkedlistEntryHeaderIsEnd(PDOUBLYLINKEDLIST_ENTRY_HEADER plist, PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
    if(pentry == plist)
        return true;
    return false;
}

// internal API Entry

static void i_doublylinkedlistEntryInit(PDOUBLYLINKEDLIST_ENTRY pentry) {
	i_doublylinkedlistEntryHeaderInit(&pentry->_header);
}

static PDOUBLYLINKEDLIST_ENTRY i_doublylinkedlistEntryNext(PDOUBLYLINKEDLIST_ENTRY pentry) {
    return (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pentry->_header);
}

static PDOUBLYLINKEDLIST_ENTRY i_doublylinkedlistEntryPrev(PDOUBLYLINKEDLIST_ENTRY pentry) {
    return (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderPrev(&pentry->_header);
}

static void i_doublylinkedlistEntryAdd(PDOUBLYLINKEDLIST_ENTRY_HEADER plist, PDOUBLYLINKEDLIST_ENTRY pentry) {
	i_doublylinkedlistEntryHeaderAdd(plist, &pentry->_header);
}

static void i_doublylinkedlistEntryDel(PDOUBLYLINKEDLIST_ENTRY pentry) {
	i_doublylinkedlistEntryHeaderDel(&pentry->_header);
}

static bool i_doublylinkedlistEntryIsEnd(PDOUBLYLINKEDLIST_ENTRY_HEADER plist, PDOUBLYLINKEDLIST_ENTRY pentry) {
    return i_doublylinkedlistEntryHeaderIsEnd(plist, &pentry->_header);
}

#define movePtrToUserData(ptr) ((void*)((size_t)ptr+sizeof(DOUBLYLINKEDLIST_ENTRY)))
#define moveUserDataToPtr(ptr) ((void*)((size_t)ptr-sizeof(DOUBLYLINKEDLIST_ENTRY)))

// external API

PDOUBLYLINKEDLIST doublylinkedlistAlloc(void) {
	PDOUBLYLINKEDLIST plist = (PDOUBLYLINKEDLIST)malloc(sizeof(DOUBLYLINKEDLIST));
	if(plist == NULL)
		return NULL;
	i_doublylinkedlistEntryHeaderInit(&plist->_activeEntries);
	i_doublylinkedlistEntryHeaderInit(&plist->_deletedEntries);
	plist->_isActiveEntriesLocked = false;
	plist->_isDeletedEntriesLocked = false;
	return plist;
}

void doublylinkedlistFree(PDOUBLYLINKEDLIST pdoublylinkedlist) {
        PDOUBLYLINKEDLIST_ENTRY pentry;
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	spinlockLock(&pdoublylinkedlist->_isDeletedEntriesLocked);
	//releasing memory from active entries
	pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_activeEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_activeEntries, pentry)) {
		i_doublylinkedlistEntryDel(pentry);
                free(pentry);
		pentry = i_doublylinkedlistEntryNext(&pdoublylinkedlist->_activeEntries);
	}
	//releasing memory from inactive entries
	pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_deletedEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_deletedEntries, pentry)) {
		i_doublylinkedlistEntryDel(pentry);
		free(pentry);
                pentry = i_doublylinkedlistEntryNext(&pdoublylinkedlist->_deletedEntries);
	}
	// don't need to release lock as structure is freed up
	free(pdoublylinkedlist);
}

void doublylinkedlistFreeDeletedEntries(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	spinlockLock(&pdoublylinkedlist->_isDeletedEntriesLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_deletedEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_deletedEntries, pentry)) {
		if(pentry->_isDeleted && pentry->_references == 0) {
			i_doublylinkedlistEntryDel(pentry);
			free(pentry);
		}
		pentry = i_doublylinkedlistEntryNext(pentry);
	}
	spinlockUnlock(&pdoublylinkedlist->_isDeletedEntriesLocked);
}

void* doublylinkedlistAdd(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key, void* ptr, size_t size) {
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)malloc(sizeof(DOUBLYLINKEDLIST_ENTRY)+size);
	if(pentry == NULL)
		return NULL;
	i_doublylinkedlistEntryInit(pentry);
	pentry->_references = 1;
	pentry->_isDeleted = false;
	pentry->_key = key;
	pentry->_size = size;
	memcpy(movePtrToUserData(pentry), ptr, size);
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	i_doublylinkedlistEntryAdd(&pdoublylinkedlist->_activeEntries, pentry);
	spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
	return movePtrToUserData(pentry);
}

void doublylinkedlistDel(PDOUBLYLINKEDLIST pdoublylinkedlist, void* ptr){
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = moveUserDataToPtr(ptr);
	i_doublylinkedlistEntryDel(pentry);
	if(pentry->_references > 0) {
		pentry->_isDeleted = true;
		spinlockLock(&pdoublylinkedlist->_isDeletedEntriesLocked);
		i_doublylinkedlistEntryAdd(&pdoublylinkedlist->_deletedEntries, pentry);
		spinlockUnlock(&pdoublylinkedlist->_isDeletedEntriesLocked);
	}
	spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
	if(pentry->_references == 0) {
		free(pentry);
	}
}

void* doublylinkedlistFind(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key) {
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_activeEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_activeEntries, pentry)) {
		if(pentry->_key == key) {
			__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
			spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
			return movePtrToUserData(pentry);
		}
		pentry = i_doublylinkedlistEntryNext(pentry);
	}
	spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
	return NULL;
}

void* doublylinkedlistGetFirst(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_activeEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_activeEntries, pentry)) {
		if(!pentry->_isDeleted) {
			__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
			spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
			return movePtrToUserData(pentry);
		}
		pentry = i_doublylinkedlistEntryNext(pentry);
	}
	__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
	spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
	return NULL;
}

void* doublylinkedlistGetLast(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderPrev(&pdoublylinkedlist->_activeEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_activeEntries, pentry)) {
		if(!pentry->_isDeleted) {
			__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
			spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
			return movePtrToUserData(pentry);
		}
		pentry = i_doublylinkedlistEntryPrev(pentry);
	}
	__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
	spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
	return NULL;
}

void doublylinkedlistRelease(void* ptr) {
	PDOUBLYLINKEDLIST_ENTRY pentry = moveUserDataToPtr(ptr);
	uint32_t new_val = __atomic_sub_fetch(&pentry->_references, 1, __ATOMIC_RELEASE);
	if(new_val == 0 && pentry->_isDeleted) {
		free(pentry);
	}
}

bool doublylinkedlistIsEmpty(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	return i_doublylinkedlistEntryHeaderIsEmpty(&pdoublylinkedlist->_activeEntries) || i_doublylinkedlistEntryHeaderIsEmpty(&pdoublylinkedlist->_deletedEntries);
}

#define movePtrToQueryUserData(ptr) ((void*)((size_t)ptr+sizeof(DOUBLYLINKEDLIST_QUERY)))
bool i_doublylinkedlistQuerySingleList(PDOUBLYLINKEDLIST_ENTRY_HEADER plist_base, PDOUBLYLINKEDLIST_QUERY *pquery, size_t *psize_inout, size_t *pcurrent_size) {
	size_t size_out = *pcurrent_size;
	PDOUBLYLINKEDLIST_QUERY pqueryElement = *pquery;
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(plist_base);
	while(!i_doublylinkedlistEntryIsEnd(plist_base, pentry)) {
		//last 8 bytes (size_t) are for ending null pointer
		if(size_out+sizeof(DOUBLYLINKEDLIST_QUERY)+pentry->_size+sizeof(size_t) > *psize_inout) {
			*pcurrent_size = size_out;
			*pquery = pqueryElement;
			return false;
		}
		pqueryElement->_key = pentry->_key;
		pqueryElement->_references = pentry->_references;
		pqueryElement->_isDeleted = pentry->_isDeleted;
		pqueryElement->_size = pentry->_size;
		memcpy(movePtrToQueryUserData(pqueryElement), movePtrToUserData(pentry), pentry->_size);
		pqueryElement->_p_userData = movePtrToQueryUserData(pqueryElement);
		size_out += sizeof(DOUBLYLINKEDLIST_QUERY)+pentry->_size;
		pqueryElement = doublylinkedlistQueryNext(pqueryElement);
		pentry = i_doublylinkedlistEntryNext(pentry);
	}
	*pcurrent_size = size_out;
	*pquery = pqueryElement;
	return true;
}

bool doublylinkedlistQuery(PDOUBLYLINKEDLIST pdoublylinkedlist, PDOUBLYLINKEDLIST_QUERY pquery, size_t *psize_inout) {
	size_t size_out = 0;
	bool activeRecordsArePulled = false, deletedRecordsArePulled = false;
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	if(i_doublylinkedlistQuerySingleList(&pdoublylinkedlist->_activeEntries, &pquery, psize_inout, &size_out)) {
	    activeRecordsArePulled = true;
        }
	spinlockUnlock(&pdoublylinkedlist->_isActiveEntriesLocked);
	if(activeRecordsArePulled) {
	    spinlockLock(&pdoublylinkedlist->_isDeletedEntriesLocked);
	    if(!i_doublylinkedlistQuerySingleList(&pdoublylinkedlist->_deletedEntries, &pquery, psize_inout, &size_out)) {
	        deletedRecordsArePulled = true;
            }
	    spinlockUnlock(&pdoublylinkedlist->_isDeletedEntriesLocked);
	}
        memset(pquery, 0, sizeof(void*));
	*psize_inout = size_out;
	return activeRecordsArePulled && deletedRecordsArePulled;
}

PDOUBLYLINKEDLIST_QUERY doublylinkedlistQueryNext(PDOUBLYLINKEDLIST_QUERY pquery) {
	return (PDOUBLYLINKEDLIST_QUERY)((size_t)pquery+sizeof(DOUBLYLINKEDLIST_QUERY)+pquery->_size);
}

bool doublylinkedlistQueryIsEnd(PDOUBLYLINKEDLIST_QUERY pquery) {
	void* ptr = *((void**)pquery);
	return (ptr == NULL);
}

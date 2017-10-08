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
	spinlockLock(&pdoublylinkedlist->_isActiveEntriesLocked);
	spinlockLock(&pdoublylinkedlist->_isDeletedEntriesLocked);
	//releasing memory from active entries
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_activeEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_activeEntries, pentry)) {
		i_doublylinkedlistEntryDel(pentry);
		pentry = i_doublylinkedlistEntryNext(pentry);
	}
	//releasing memory from inactive entries
	pentry = (PDOUBLYLINKEDLIST_ENTRY)i_doublylinkedlistEntryHeaderNext(&pdoublylinkedlist->_deletedEntries);
	while(!i_doublylinkedlistEntryIsEnd(&pdoublylinkedlist->_deletedEntries, pentry)) {
		i_doublylinkedlistEntryDel(pentry);
		pentry = i_doublylinkedlistEntryNext(pentry);
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

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistAdd(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key, void* ptr, size_t size) {
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)malloc(sizeof(DOUBLYLINKEDLIST_ENTRY)+size);
	if(pentry == NULL)
		return NULL;
	i_doublylinkedlistEntryInit(pentry);
	pentry->_references = 1;
	pentry->_isDeleted = false;
	pentry->_key = key;
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

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistFind(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key) {
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

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistGetFirst(PDOUBLYLINKEDLIST pdoublylinkedlist) {
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

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistGetLast(PDOUBLYLINKEDLIST pdoublylinkedlist) {
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

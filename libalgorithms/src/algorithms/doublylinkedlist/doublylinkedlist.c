#include "doublylinkedlist.h"
#include <stdlib.h>

// internal API Entry Header

static void i_doublylinkedlistEntyHeaderInit(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
	pentry->_next = pentry;
	pentry->_prev = pentry;
}

static PDOUBLYLINKEDLIST_ENTRY_HEADER i_doublylinkedlistEntyHeaderNext(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
    return pentry->_next;
}

static PDOUBLYLINKEDLIST_ENTRY_HEADER i_doublylinkedlistEntyHeaderPrev(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
    return pentry->_prev;
}

static void i_doublylinkedlistEntyHeaderAdd(PDOUBLYLINKEDLIST_ENTRY_HEADER plist, PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
	pentry->_next = plist->_next;
    pentry->_prev = plist;
    plist->_next->_prev = pentry;
    plist->_next = pentry;
}

static void i_doublylinkedlistEntyHeaderDel(PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
	pentry->_prev->_next = pentry->_next;
	pentry->_next->_prev = pentry->_prev;
	i_doublylinkedlistEntyHeaderInit(pentry);
}

static bool i_doublylinkedlistEntyHeaderIsEmpty(PDOUBLYLINKEDLIST_ENTRY_HEADER plist) {
    if(plist->_next == plist)
        return true;
    return false;
}

static bool i_doublylinkedlistEntyHeaderIsEnd(PDOUBLYLINKEDLIST_ENTRY_HEADER plist, PDOUBLYLINKEDLIST_ENTRY_HEADER pentry) {
    if(pentry == plist)
        return true;
    return false;
}

// external API

PDOUBLYLINKEDLIST doublylinkedlistAlloc(void) {
	PDOUBLYLINKEDLIST plist = (PDOUBLYLINKEDLIST)malloc(sizeof(DOUBLYLINKEDLIST));
	if(plist == NULL)
		return NULL;
	i_doublylinkedlistEntyHeaderInit(plist);
	plist->_isLocked = false;
	return plist;
}

void doublylinkedlistFree(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	spinlockLock(&pdoublylinkedlist->_isLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = i_doublylinkedlistEntyHeaderNext(&pdoublylinkedlist->_entries);
	while(!i_doublylinkedlistEntyHeaderIsEnd(pdoublylinkedlist, pentry)) {
		i_doublylinkedlistEntyHeaderDel(pentry);
		free(pentry);
		pentry = i_doublylinkedlistEntyHeaderNext(&pentry->_header);
	}
	// don't need to release lock as structure is freed up
	free(pdoublylinkedlist);
}

void doublylinkedlistFreeDeletedEntries(PDOUBLYLINKEDLIST pdoublylinkedlist) {
/*	spinlockLock(&pdoublylinkedlist->_isLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = i_doublylinkedlistEntyHeaderNext(&pdoublylinkedlist->_entries);
	while(!i_doublylinkedlistEntyHeaderIsEnd(pdoublylinkedlist, pentry)) {
		if(pentry->_key == key) {
			__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
			spinlockUnlock(&pdoublylinkedlist->_isLocked);
			return pentry;
		}
		pentry = i_doublylinkedlistEntyHeaderNext(&pentry->_header);
	}
__cleanup:
	spinlockUnlock(&pdoublylinkedlist->_isLocked);*/
}

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistAdd(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key, void* ptr, size_t size) {
	PDOUBLYLINKEDLIST_ENTRY pentry = (PDOUBLYLINKEDLIST_ENTRY)malloc(sizeof(DOUBLYLINKEDLIST_ENTRY)+size);
	if(pentry == NULL)
		return NULL;
	i_doublylinkedlistEntyHeaderInit(pentry);
	pentry->_references = 1;
	pentry->_isDeleted = false;
	pentry->_key = key;
	memcpy((void*)((uint32_t)pentry+sizeof(DOUBLYLINKEDLIST_ENTRY)), ptr, size);
	spinlockLock(&pdoublylinkedlist->_isLocked);
	i_doublylinkedlistEntyHeaderAdd(&pdoublylinkedlist->_entries, pentry);
	spinlockUnlock(&pdoublylinkedlist->_isLocked);
	return pentry;
}

void doublylinkedlistDel(PDOUBLYLINKEDLIST pdoublylinkedlist, PDOUBLYLINKEDLIST_ENTRY pentry){
	spinlockLock(&pdoublylinkedlist->_isLocked);
	i_doublylinkedlistEntyHeaderDel(pentry);
	if(pentry->_references > 0) {
		pentry->_isDeleted = true;
	}
	spinlockUnlock(&pdoublylinkedlist->_isLocked);
	if(pentry->_references == 0) {
		free(pentry);
	}
}

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistFind(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key) {
	spinlockLock(&pdoublylinkedlist->_isLocked);
	PDOUBLYLINKEDLIST_ENTRY pentry = i_doublylinkedlistEntyHeaderNext(&pdoublylinkedlist->_entries);
	while(!i_doublylinkedlistEntyHeaderIsEnd(pdoublylinkedlist, pentry)) {
		if(pentry->_key == key) {
			__atomic_add_fetch(&pentry->_references, 1, __ATOMIC_ACQUIRE);
			spinlockUnlock(&pdoublylinkedlist->_isLocked);
			return pentry;
		}
		pentry = i_doublylinkedlistEntyHeaderNext(&pentry->_header);
	}
__cleanup:
	spinlockUnlock(&pdoublylinkedlist->_isLocked);
	return NULL;
}

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistGetFirst(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	return i_doublylinkedlistEntyHeaderNext(&pdoublylinkedlist->_entries);
}

PDOUBLYLINKEDLIST_ENTRY doublylinkedlistGetLast(PDOUBLYLINKEDLIST pdoublylinkedlist) {
	return i_doublylinkedlistEntyHeaderPrev(&pdoublylinkedlist->_entries);
}

void doublylinkedlistRelease(PDOUBLYLINKEDLIST_ENTRY pentry) {
	uint32_t new_val = __atomic_sub_fetch(&pentry->_references, 1, __ATOMIC_RELEASE);
	if(new_val == 0 && pentry->_isDeleted) {
		free(pentry);
	}
}

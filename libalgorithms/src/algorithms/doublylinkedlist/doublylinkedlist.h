#ifndef _LIBALGORITHMS_ALGORITHMS_DOUBLYLINKEDLIST_H
#define _LIBALGORITHMS_ALGORITHMS_DOUBLYLINKEDLIST_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct _DOUBLYLINKEDLIST_ENTRY_HEADER {
	struct _DOUBLYLINKEDLIST_ENTRY_HEADER* _next;
	struct _DOUBLYLINKEDLIST_ENTRY_HEADER* _prev;
} DOUBLYLINKEDLIST_ENTRY_HEADER, *PDOUBLYLINKEDLIST_ENTRY_HEADER;

typedef struct _DOUBLYLINKEDLIST_ENTRY {
	DOUBLYLINKEDLIST_ENTRY_HEADER _header;
	uint64_t _key;
	uint32_t _references;
	bool _isDeleted;
} DOUBLYLINKEDLIST_ENTRY, *PDOUBLYLINKEDLIST_ENTRY;

typedef struct _DOUBLYLINKEDLIST {
	DOUBLYLINKEDLIST_ENTRY_HEADER _activeEntries;
	bool _isActiveEntriesLocked;
	DOUBLYLINKEDLIST_ENTRY_HEADER _deletedEntries;
	bool _isDeletedEntriesLocked;
} DOUBLYLINKEDLIST, *PDOUBLYLINKEDLIST;

PDOUBLYLINKEDLIST doublylinkedlistAlloc(void);
void doublylinkedlistFree(PDOUBLYLINKEDLIST pdoublylinkedlist);
void doublylinkedlistFreeDeletedEntries(PDOUBLYLINKEDLIST pdoublylinkedlist);
PDOUBLYLINKEDLIST_ENTRY doublylinkedlistAdd(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key, void* ptr, size_t size);
void doublylinkedlistDel(PDOUBLYLINKEDLIST pdoublylinkedlist, void* ptr);
PDOUBLYLINKEDLIST_ENTRY doublylinkedlistFind(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key);
PDOUBLYLINKEDLIST_ENTRY doublylinkedlistGetFirst(PDOUBLYLINKEDLIST pdoublylinkedlist);
PDOUBLYLINKEDLIST_ENTRY doublylinkedlistGetLast(PDOUBLYLINKEDLIST pdoublylinkedlist);
void doublylinkedlistRelease(void* ptr);
bool doublylinkedlistIsEmpty(PDOUBLYLINKEDLIST pdoublylinkedlist);
#endif

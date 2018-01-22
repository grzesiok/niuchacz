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
	size_t _size;
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
void* doublylinkedlistAdd(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key, void* ptr, size_t size);
void doublylinkedlistDel(PDOUBLYLINKEDLIST pdoublylinkedlist, void* ptr);
void* doublylinkedlistFind(PDOUBLYLINKEDLIST pdoublylinkedlist, uint64_t key);
void* doublylinkedlistGetFirst(PDOUBLYLINKEDLIST pdoublylinkedlist);
void* doublylinkedlistGetLast(PDOUBLYLINKEDLIST pdoublylinkedlist);
void doublylinkedlistRelease(void* ptr);
bool doublylinkedlistIsEmpty(PDOUBLYLINKEDLIST pdoublylinkedlist);

//Querying mechanism

typedef struct _DOUBLYLINKEDLIST_QUERY {
	uint64_t _key;
	uint32_t _references;
	bool _isDeleted;
	size_t _size;
	void* _p_userData;
} DOUBLYLINKEDLIST_QUERY, *PDOUBLYLINKEDLIST_QUERY;

bool doublylinkedlistQuery(PDOUBLYLINKEDLIST pdoublylinkedlist, PDOUBLYLINKEDLIST_QUERY pquery, size_t *psize_inout);
PDOUBLYLINKEDLIST_QUERY doublylinkedlistQueryNext(PDOUBLYLINKEDLIST_QUERY pquery);
bool doublylinkedlistQueryIsEnd(PDOUBLYLINKEDLIST_QUERY pquery);
#endif

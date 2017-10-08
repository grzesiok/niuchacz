#include <stdlib.h>
#include <stdio.h>
#include "../src/algorithms/doublylinkedlist/doublylinkedlist.h"

typedef struct _TEST_ENTRY {
	uint32_t _val;
} *PTEST_ENTRY, TEST_ENTRY;

char g_buff[4096];

int main() {
	PDOUBLYLINKEDLIST plist;
	uint32_t tab[] = {1, 2, 3, 4, 5};
	const uint32_t tab_maxsize = sizeof(tab)/sizeof(tab[0]);
	uint32_t i;
	PDOUBLYLINKEDLIST_QUERY pquery;
	const size_t current_size = sizeof(g_buff);
	size_t required_size;

	printf("LIST init...\n");
	plist = doublylinkedlistAlloc();
	if(plist == NULL) {
		perror("Can't alloc doublylinkedlist!");
		return 1;
	}
	printf("LIST created\n");
	printf("Adding entries...\n");
	for(i = 0;i < tab_maxsize;i++) {
		printf("Adding val=%u\n", tab[i]);
		PTEST_ENTRY pentry = (PTEST_ENTRY)doublylinkedlistAdd(plist, tab[i], &tab[i], sizeof(TEST_ENTRY));
		printf("Adding pentry=%p\n", pentry);
		doublylinkedlistRelease(pentry);
	}
	printf("Querying entries...\n");
	pquery = (PDOUBLYLINKEDLIST_QUERY)g_buff;
	required_size = current_size;
	if(!doublylinkedlistQuery(plist, pquery, &required_size)) {
		printf("Too much entries! curr_size=%lu required_size=%lu\n", current_size, required_size);
		return 1;
	}
	while(!doublylinkedlistQueryIsEnd(pquery)) {
		printf("Entry _key=%lu _references=%u _isDeleted=%c _size=%lu\n", pquery->_key, pquery->_references, (pquery->_isDeleted) ? 'Y' : 'N', pquery->_size);
		pquery = doublylinkedlistQueryNext(pquery);
	}
	printf("Checking values...\n");
	for(i = 0;i < tab_maxsize;i++) {
		PTEST_ENTRY pentry = (PTEST_ENTRY)doublylinkedlistFind(plist, tab[i]);
		printf("pentry tab_val=%u val=%u\n", tab[i], pentry->_val);
		if(pentry == NULL) {
			perror("Error during pulling value from list!");
			return 1;
		}
		doublylinkedlistRelease(pentry);
		printf("pentry pentry=%p tab_val=%u val=%u\n", pentry, tab[i], pentry->_val);
		doublylinkedlistDel(plist, pentry);
		printf("pentry deleted\n");
	}
	printf("Querying entries...\n");
	pquery = (PDOUBLYLINKEDLIST_QUERY)g_buff;
	required_size = current_size;
	if(!doublylinkedlistQuery(plist, pquery, &required_size)) {
		printf("Too much entries! curr_size=%lu required_size=%lu\n", current_size, required_size);
		return 1;
	}
	while(!doublylinkedlistQueryIsEnd(pquery)) {
		printf("Entry _key=%lu _references=%u _isDeleted=%c _size=%lu\n", pquery->_key, pquery->_references, (pquery->_isDeleted) ? 'Y' : 'N', pquery->_size);
		pquery = doublylinkedlistQueryNext(pquery);
	}
	if(doublylinkedlistIsEmpty(plist)) {
		printf("LIST empty\n");
	} else {
		printf("LIST not empty!\n");
		return 1;
	}
	doublylinkedlistFree(plist);
	return 0;
}

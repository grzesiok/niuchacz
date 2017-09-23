#include <stdlib.h>
#include "../src/algorithms/doublylinkedlist/doublylinkedlist.h"

typedef struct _TEST_ENTRY {
	DOUBLYLINKEDLIST_ENTRY _header;
	uint32_t _val;
} *PTEST_ENTRY, TEST_ENTRY;

int main() {
	PDOUBLYLINKEDLIST plist;
	uint32_t tab[] = {1, 2, 3, 4, 5};
	uint32_t tab_maxsize = sizeof(tab)/sizeof(tab[0]);
	uint32_t i;

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
		doublylinkedlistAdd(plist, tab[i], &tab[i], sizeof(tab[i]));
	}
	printf("Checking values...\n");
	for(i = 0;i < tab_maxsize;i++) {
		PTEST_ENTRY pentry = (PTEST_ENTRY)doublylinkedlistFind(plist, tab[i]);
		if(pentry == NULL) {
			perror("Error during pulling value from list!");
			return 1;
		}
		printf("pentry val=%u\n", pentry->_val);
		doublylinkedlistDel(plist, pentry);
		printf("pentry deleted\n");
	}
	printf("LIST empty\n");
	doublylinkedlistFree(plist);
	return 0;
}

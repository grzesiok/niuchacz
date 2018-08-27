#include <stdlib.h>
#include <stdio.h>
#include "../src/algorithms/tree/bst.h"

typedef struct _TEST_ENTRY {
    uint32_t _val;
} *PTEST_ENTRY, TEST_ENTRY;

int main() {
    bst_t* pbst;
    uint32_t tab_insert[] = {50, 30, 70, 20, 40, 60, 80};
    uint32_t tab_delete[] = {20, 30, 50, 1, 40, 80, 60, 70, 2};
    uint32_t i;
    int ret;
	
    printf("BST create...\n");
    pbst = bst_create();
    if(pbst == NULL) {
        perror("Can't alloc BST!");
        return 1;
    }
    printf("BST created\n");
    printf("Adding entries...\n");
    for(i = 0;i < sizeof(tab_insert)/sizeof(tab_insert[0]);i++) {
        printf("Adding val=%u\n", tab_insert[i]);
        ret =  bst_insert(pbst, tab_insert[i], &tab_insert[i], sizeof(uint32_t), NULL);
        if(ret == -1) {
            printf("Error during inserting data!\n");
            return -1;
        }
    }
    bst_print(pbst->_root, 0);
    for(i = 0;i < sizeof(tab_delete)/sizeof(tab_delete[0]);i++) {
        printf("Deleting val=%u\n", tab_delete[i]);
        bst_delete(pbst, tab_delete[i]);
        bst_print(pbst->_root, 0);
    }
    printf("BST destroy...\n");
    bst_destroy(pbst);
    return 0;
}

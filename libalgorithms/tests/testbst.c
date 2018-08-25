#include <stdlib.h>
#include <stdio.h>
#include "../src/algorithms/tree/bst.h"

typedef struct _TEST_ENTRY {
    uint32_t _val;
} *PTEST_ENTRY, TEST_ENTRY;

int main() {
    bst_t* pbst;
    uint32_t tab[] = {3, 2, 1, 4, 5};
    const uint32_t tab_maxsize = sizeof(tab)/sizeof(tab[0]);
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
    for(i = 0;i < tab_maxsize;i++) {
        printf("Adding val=%u\n", tab[i]);
        ret =  bst_insert(pbst, tab[i], &tab[i], sizeof(uint32_t), NULL);
        if(ret == -1) {
            printf("Error during inserting data!\n");
            return -1;
        }
    }
    bst_print(pbst->_root, 0);
    printf("BST destroy...\n");
    bst_destroy(pbst);
    return 0;
}

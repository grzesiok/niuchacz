#include "bst.h"
#include "../../../include/math.h"
#include "../../../include/memory.h"
#include <stdlib.h>
#include <string.h>

// internal API

bst_node_t* i_bst_alloc_node(uint64_t key, size_t dataSize) {
    bst_node_t *pbst_node;
    pbst_node = (bst_node_t*)malloc(sizeof(bst_node_t)+dataSize);
    if(!pbst_node)
        return NULL;
    pbst_node->_key = key;
    pbst_node->_left = NULL;
    pbst_node->_right = NULL;
    pbst_node->_dataSize = dataSize;
    return pbst_node;
}

bst_node_t* i_bst_clone_node(bst_node_t* pbst_node, size_t dataSize) {
    bst_node_t* pbst_node_cloned;
    pbst_node_cloned = i_bst_alloc_node(pbst_node->_key, dataSize);
    pbst_node_cloned->_left = pbst_node->_left;
    pbst_node_cloned->_right = pbst_node->_right;
    free(pbst_node);
    return pbst_node_cloned;
}

// external API

bst_t* bst_create(void) {
    bst_t *pbst = malloc(sizeof(bst_t));
    if(pbst == NULL)
        return NULL;
    pbst->_root = NULL;
    return pbst;
}

void bst_destroy(bst_t* pbst) {
    free(pbst);
}

int bst_insert(bst_t *pbst, uint64_t key, const void *pbuf, size_t nBytes, const struct timespec *dataTimeout) {
    bst_node_t* pbst_node = pbst->_root;
    bst_node_t* pbst_parentnode = NULL;
    while(pbst_node) {
        if(pbst_node->_key == key) {
            if(nBytes <= pbst_node->_dataSize) {
                memcpy(memoryPtrMove(pbst_node, sizeof(bst_node_t)), pbuf, nBytes);
                return nBytes;
            } else {
                pbst_node = i_bst_clone_node(pbst_node, nBytes);
                goto __insert;
            }
        }
        pbst_parentnode = pbst_node;
        if(pbst_node->_key > key) {
            pbst_node = pbst_node->_left;
        } else {
            pbst_node = pbst_node->_right;
        }
    }
    pbst_node = i_bst_alloc_node(key, nBytes);
    memcpy(memoryPtrMove(pbst_node, sizeof(bst_node_t)), pbuf, nBytes);
    if(pbst->_root == NULL) {
        pbst->_root = pbst_node;
        return nBytes;
    }
__insert:
    if(pbst_parentnode->_key > key) {
        pbst_parentnode->_left = pbst_node;
    } else {
        pbst_parentnode->_right = pbst_node;
    }
    return nBytes;
}

int bst_delete(bst_t *pbst, uint64_t key) {
	return -1;
}

size_t bst_search(bst_t *pbst, uint64_t key, void *pbuf, size_t nMaxBytes) {
    bst_node_t *pbst_node;
    pbst_node = pbst->_root;
    while(pbst_node) {
        if(pbst_node->_key == key) {
            break;
        }
        if(pbst_node->_key > key) {
            pbst_node = pbst_node->_left;
        } else {
            pbst_node = pbst_node->_right;
        }
    }
    if(pbst_node == NULL)
        return -1;
    memcpy(pbuf, memoryPtrMove(pbst_node, sizeof(bst_node_t)), MIN(nMaxBytes, pbst_node->_dataSize));
    return MIN(nMaxBytes, pbst_node->_dataSize);
}

void bst_print(bst_node_t *pbst_node, int level) {
    int i;
    if(pbst_node == NULL)
        return;
    for(i = 0;i < level;i++)
        printf("-");
    printf("%llu\n", pbst_node->_key);
    bst_print(pbst_node->_left, level+1);
    bst_print(pbst_node->_right, level+1);
}

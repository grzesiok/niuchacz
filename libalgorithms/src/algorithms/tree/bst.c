#include "bst.h"
#include "../../../include/math.h"
#include "../../../include/memory.h"
#include "../timer/timer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

bst_node_t* i_bst_min_value(bst_node_t* pbst_node) {
    if(pbst_node == NULL)
        return NULL;
    while(pbst_node->_left) {
        pbst_node = pbst_node->_left;
    }
    return pbst_node;
}

bst_node_t* i_bst_delete_node(bst_node_t* root, int key) {
    // base case
    if(root == NULL) return root;
    // If the key to be deleted is smaller than the root's key,
    // then it lies in left subtree
    if(key < root->_key)
        root->_left = i_bst_delete_node(root->_left, key);
    // If the key to be deleted is greater than the root's key,
    // then it lies in right subtree
    else if(key > root->_key)
        root->_right = i_bst_delete_node(root->_right, key);
    // if key is same as root's key, then This is the node
    // to be deleted
    else {
        // node with only one child or no child
        if(root->_left == NULL) {
            bst_node_t *temp = root->_right;
            free(root);
            return temp;
        } else if(root->_right == NULL) {
            bst_node_t *temp = root->_left;
            free(root);
            return temp;
        }
        // node with two children: Get the inorder successor (smallest
        // in the right subtree)
        bst_node_t* temp = i_bst_min_value(root->_right);
        // Copy the inorder successor's content to this node
        root->_key = temp->_key;
        // Delete the inorder successor
        root->_right = i_bst_delete_node(root->_right, temp->_key);
    }
    return root;
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
    if(dataTimeout == NULL) {
        memset(pbst_node, 0, sizeof(struct timespec));
    } else pbst_node->_dataTimeout = *dataTimeout;
    if(pbst_parentnode->_key > key) {
        pbst_parentnode->_left = pbst_node;
    } else {
        pbst_parentnode->_right = pbst_node;
    }
    return nBytes;
}

void bst_delete(bst_t *pbst, uint64_t key) {
    pbst->_root = i_bst_delete_node(pbst->_root, key);
}

size_t bst_search(bst_t *pbst, uint64_t key, void *pbuf, size_t nMaxBytes) {
    bst_node_t *pbst_node;
    pbst_node = pbst->_root;
    struct timespec ts;
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
    if(!timerIsNull(&pbst_node->_dataTimeout)) {
        timerGetRealCurrentTimestamp(&ts);
        if(timerCmp(&ts, &pbst_node->_dataTimeout) < 0) {
            bst_delete(pbst, key);
            return -2;
        }
    }
    if(pbst_node == NULL)
        return -1;
    memcpy(pbuf, memoryPtrMove(pbst_node, sizeof(bst_node_t)), MIN(nMaxBytes, pbst_node->_dataSize));
    return MIN(nMaxBytes, pbst_node->_dataSize);
}

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
void bst_print(bst_node_t *pbst_node, int level) {
    int i;
    if(pbst_node == NULL)
        return;
    for(i = 0;i < level;i++)
        printf("-");
    printf("%"PRIu64"\n", pbst_node->_key);
    bst_print(pbst_node->_left, level+1);
    bst_print(pbst_node->_right, level+1);
}

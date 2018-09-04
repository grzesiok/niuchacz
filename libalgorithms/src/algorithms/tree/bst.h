#ifndef _LIBALGORITHMS_ALGORITHMS_TREE_BST_H
#define _LIBALGORITHMS_ALGORITHMS_TREE_BST_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct _bst_node_t {
    uint64_t _key;
    struct _bst_node_t* _left;
    struct _bst_node_t* _right;
    struct timespec _dataTimeout;
    size_t _dataSize;
} bst_node_t;

typedef bool (*bst_expire_handler_t)(uint64_t key, void *pbuf, size_t nBytes);

typedef struct {
    bst_node_t* _root;
    bst_expire_handler_t _expireHandler;
} bst_t;

bst_t* bst_create(bst_expire_handler_t expireHandler);
void bst_destroy(bst_t* pbst);
int bst_insert(bst_t *pbst, uint64_t key, const void *pbuf, size_t nBytes, const struct timespec *dataTimeout);
void bst_delete(bst_t *pbst, uint64_t key);
size_t bst_search(bst_t *pbst, uint64_t key, void *pbuf, size_t nMaxBytes);
void bst_print(bst_node_t* pbst_node, int level);

#endif /*_LIBALGORITHMS_ALGORITHMS_TREE_BST_H */

#ifndef _LIBALGORITHMS_ALGORITHMS_TREE_BST_H
#define _LIBALGORITHMS_ALGORITHMS_TREE_BST_H

#include <stddef.h>
#include <stdint.h>
#include "../doublylinkedlist/doublylinkedlist.h"

typedef struct {
	struct timespec _dataTimeout;
	size_t _dataSize;
} bst_node_entry_t;

typedef struct {
	uint64_t _key;
	struct bst_node_t* _left;
	struct bst_node_t* _right;
	PDOUBLYLINKEDLIST _entryList;
} bst_node_t;

typedef struct {
    size_t _maxsize;
    bst_node_t* _root;
} bst_t;

bst_t* bst_create(size_t size);
void bst_destroy(bst_t* pbst);
int bst_insert(bst_t *pbst, uint64_t key, const void *pbuf, size_t nBytes, const struct timespec *dataTimeout);
int bst_delete(bst_t *pbst, uint64_t key);
size_t bst_search(bst_t *pbst, uint64_t key, void *pbuf, size_t nMaxBytes);


#endif /*_LIBALGORITHMS_ALGORITHMS_TREE_BST_H */

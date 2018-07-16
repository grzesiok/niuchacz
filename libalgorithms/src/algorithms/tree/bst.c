#include "bst.h"
#include "../../../include/math.h"
#include <stdlib.h>
#include <string.h>

// internal API

bst_node_t* i_bst_alloc_node(uint64_t key) {
	bst_node_t *pbst_node;
	pbst_node = (bst_node_t*)malloc(sizeof(bst_node_t));
	if(!pbst_node)
		return NULL;
	pbst_node->_key = key;
	pbst_node->_left = NULL;
	pbst_node->_right = NULL;
	pbst_node->_entryList = doublylinkedlistAlloc();
	if(!pbst_node->_entryList) {
		free(pbst_node);
		return NULL;
	}
	return pbst_node;
}

// external API

bst_t* bst_create(size_t size) {
	bst_t *pbst_tmp = malloc(sizeof(bst_t));
	if(pbst_tmp == NULL)
		return NULL;
	pbst_tmp->_maxsize = size;
	pbst_tmp->_root = NULL;
	return pbst_tmp;
}

void bst_destroy(bst_t* pbst) {
	free(pbst);
}

int bst_insert(bst_t *pbst, uint64_t key, const void *pbuf, size_t nBytes, const struct timespec *dataTimeout) {
	bst_node_t *pbst_parentnode = NULL;
	bst_node_t *pbst_node;

	if(!pbst->_root) {
		pbst_node = i_bst_alloc_node(key);
		if(pbst_node == NULL)
			return -1;
		pbst->_root = pbst_node;
	} else {
		pbst_node = pbst->_root;
		while(pbst_node) {
			if(pbst_node->_key == key) {
				goto __insert_node_entry;
			}
			pbst_parentnode = pbst_node;
			if(pbst_node->_key > key) {
				pbst_node = pbst_node->_left;
			} else {
				pbst_node = pbst_node->_right;
			}
		}
__insert_node:
		if(pbst_node == NULL) {
			pbst_node = i_bst_alloc_node(key);
			if(pbst_node == NULL)
				return -1;
			if(pbst_parentnode->_key > key) {
				pbst_parentnode->_left = pbst_node;
			} else {
				pbst_parentnode->_right = pbst_node;
			}
		}
__insert_node_entry:
		doublylinkedlistAdd(pbst_node->_entryList, key, pbuf, nBytes);
	}
	return -1;
}

int bst_delete(bst_t *pbst, uint64_t key) {
	return -1;
}

size_t bst_search(bst_t *pbst, uint64_t key, void *pbuf, size_t nMaxBytes) {
	bst_node_t *pbst_node;
	bst_node_entry_t* pbst_node_entry;

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
	pbst_node_entry = (bst_node_entry_t*)doublylinkedlistFind(pbst_node->_entryList, key);
	if(pbst_node_entry == NULL)
		return -1;
	memcpy(pbst_node_entry->_data, pbuf, MIN(nMaxBytes, pbst_node_entry->_dataSize));
	return MIN(nMaxBytes, pbst_node_entry->_dataSize);
}

#include "object_pool.h"
#include <stddef.h>


void mem_pool_init(mem_pool_t *pool, void *buffer, size_t buffer_size, size_t block_size) {
    pool->block_size = block_size;
    size_t num_blocks = buffer_size / block_size;
    char *p = (char*)buffer;
    pool_block_t **prev_next = (pool_block_t**)&pool->free_list;
    for (size_t i = 0; i < num_blocks; i++) {
        *prev_next = (pool_block_t*)p;
        prev_next = &((pool_block_t*)p)->next;
        p += block_size;
    }
    *prev_next = NULL;
}

void *mem_pool_alloc(mem_pool_t *pool) {
    if (!pool->free_list) {
        return NULL;
    }
    void *block = pool->free_list;
    pool->free_list = pool->free_list->next;
    return block;
}

void mem_pool_free(mem_pool_t *pool, void *block) {
    if (!block) return;
    pool_block_t *node = (pool_block_t*)block;
    node->next = pool->free_list;
    pool->free_list = node;
}
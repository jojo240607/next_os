//
// Created by zhiwei.gong on 2026/4/24.
//

#ifndef STM32F4DISCOVERY_LINEAR_POOL_H
#define STM32F4DISCOVERY_LINEAR_POOL_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// 线性分配器（Linear Allocator）
typedef struct {
    uint8_t *start;      // 内存池起始地址
    uint8_t *next;       // 下一个可分配地址
    size_t size;         // 总大小
} linear_allocator_t;


typedef enum :uint8_t {
    ALIGNMENT_1BYTE = 1,
    ALIGNMENT_2BYTE = 2,
    ALIGNMENT_4BYTE = 4,
    ALIGNMENT_8BYTE = 8,
    ALIGNMENT_32BYTE = 32,
} byte_alignment_t;
void os_pool_init();
void *os_malloc(size_t num_bytes) ;
void *os_ccm_malloc(size_t num_bytes, byte_alignment_t alignment);
void *os_stack_malloc(size_t num_bytes, byte_alignment_t alignment);
void os_free (void *p);
linear_allocator_t *linear_pool_get();
#endif //STM32F4DISCOVERY_LINEAR_POOL_H

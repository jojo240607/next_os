#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 环形缓冲区结构体 */
typedef struct {
    uint8_t *buffer;   // 缓冲区内存块
    size_t   size;     // 缓冲区总容量（字节数）
    size_t   head;     // 写指针（下一次写入的位置）
    size_t   tail;     // 读指针（下一次读取的位置）
    size_t   count;    // 当前已存储的字节数
    size_t   dma_prev;  // 上一次 DMA 写入的结尾位置（索引）
} RingBuf;

RingBuf* ringbuf_create(size_t size);
/* 初始化环形缓冲区 */
void ringbuf_init(RingBuf *rb, size_t size);

/* 写入一个字节，满时丢弃并返回 false */
bool ringbuf_put(RingBuf *rb, uint8_t data);

/* 读取一个字节，空时返回 false */
//bool ringbuf_get(RingBuf *rb, uint8_t *data);
size_t ringbuf_get(RingBuf *rb, uint8_t *data, size_t max_len);
void ringbuf_dma_update(RingBuf *rb, size_t new_head);
/* 查询是否为空 / 为满 */
bool ringbuf_is_empty(const RingBuf *rb);
bool ringbuf_is_full(const RingBuf *rb);

/* 获取当前数据数量 */
size_t ringbuf_count(const RingBuf *rb);

#endif

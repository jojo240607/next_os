#include "ringbuf.h"
#include "../common/linear_pool.h"

RingBuf* ringbuf_create(size_t size) {
    RingBuf* obj = (RingBuf*)os_malloc(sizeof(RingBuf));
    if (obj) {
        memset(obj, 0, sizeof(RingBuf));
        ringbuf_init(obj, size);
    }
    return obj;
}

void ringbuf_init(RingBuf *rb, size_t size)
{
    rb->buffer = os_malloc(sizeof(uint8_t) * size);
    memset(rb->buffer, 0, sizeof(uint8_t) * size);
    rb->size   = size;
    rb->head   = 0;
    rb->tail   = 0;
    rb->count  = 0;
    rb->dma_prev = 0;
}

bool ringbuf_put(RingBuf *rb, uint8_t data)
{
    if (rb->count == rb->size) {
        // 满：丢弃最旧的数据（移动 tail）
        rb->tail = (rb->tail + 1) % rb->size;
        rb->count--;  // 减去丢弃的
        // 可选：设置溢出标志或增加丢弃计数
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    return true;   // 总是成功
}

//bool ringbuf_get(RingBuf *rb, uint8_t *data)
//{
//    if (rb->count == 0) {
//        /* 缓冲区空 */
//        return false;
//    }
//    *data = rb->buffer[rb->tail];
//    rb->tail = (rb->tail + 1) % rb->size;
//    rb->count--;
//    return true;
//}

size_t ringbuf_get(RingBuf *rb, uint8_t *data, size_t max_len)
{
    if (!data) {
        return 0;
    }
    // 确定实际可读字节数：不超过请求大小，也不超过已有数据
    size_t bytes = (max_len < rb->count) ? max_len : rb->count;
    if (bytes == 0) {
        return 0;
    }

    size_t tail = rb->tail;
    size_t size = rb->size;

    // 第一段：从 tail 到缓冲区末尾（或到 bytes 限制）
    size_t first_part = size - tail;      // 从 tail 到末尾的字节数
    if (first_part > bytes) {
        first_part = bytes;               // 不需要读到底就够用
    }
    memcpy(data, &rb->buffer[tail], first_part);

    // 第二段：如果还有剩余数据，一定是从缓冲区头部开始
    size_t second_part = bytes - first_part;
    if (second_part > 0) {
        memcpy(data + first_part, rb->buffer, second_part);
    }

    // 更新读指针和数据计数
    rb->tail = (tail + bytes) % size;
    rb->count -= bytes;

    return bytes;   // 返回实际读到的字节数
}

void ringbuf_dma_update(RingBuf *rb, size_t new_head)
{
    size_t prev = rb->head;  // 上一次记录的写指针
    size_t size = rb->size;

    if (new_head == prev) return;

    size_t new_bytes;
    if (new_head > prev) {
        new_bytes = new_head - prev;
    } else {
        new_bytes = (size - prev) + new_head;
    }

    // 更新 count，但要注意覆盖可能溢出
    if (rb->count + new_bytes > size) {
        // 发生了覆盖，需要丢弃部分旧数据
        size_t overflow = (rb->count + new_bytes) - size;
        rb->tail = (rb->tail + overflow) % size;
        rb->count -= overflow;
        // 记录溢出
    }

    rb->count += new_bytes;
    rb->head = new_head;
}

bool ringbuf_is_empty(const RingBuf *rb)
{
    return (rb->count == 0);
}

bool ringbuf_is_full(const RingBuf *rb)
{
    return (rb->count == rb->size);
}

size_t ringbuf_count(const RingBuf *rb)
{
    return rb->count;
}

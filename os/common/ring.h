#ifndef RING_H
#define RING_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define GET_RING(obj) ((Ring *)obj)
// 类声明
typedef struct _Ring Ring;
typedef struct _RingFun RingFun;
// 类成员函数结构
struct _RingFun {
    void (*destroy)(Ring* self);
	bool (*push)(Ring* self, const void *data);
	bool (*pop)(Ring* self, void *data);
	void * (*pop_nocpy)(Ring* self);

};
// 类结构
struct _Ring {
    const RingFun* fun;
    // TODO: 添加数据成员
    void *buffer;           /* 底层字节数组 */
    uint32_t head;               /* 读指针（取数据位置） */
    uint32_t tail;               /* 写指针（存数据位置） */
    uint32_t capacity;           /* 最大元素个数 */
    uint32_t elem_size;          /* 每个元素的字节数 */
    uint32_t count;              /* 当前元素个数 */
};

// 构造函数声明
Ring* ring_create(uint32_t capacity, uint32_t size);
void ring_init(Ring* self, uint32_t capacity, uint32_t size);

// 析构函数声明
void ring_deinit(Ring* self);

#endif // RING_H
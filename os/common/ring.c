#include "ring.h"
#include <stdio.h>
#include "linear_pool.h"
#include "util.h"

static void * ring_pop_nocpy(Ring* self);

static bool ring_push(Ring* self, const void *data);
static bool ring_pop(Ring* self, void *data);

// 析构函数声明
static void ring_destroy(Ring* self);

// TODO: 初始化数据成员
static const RingFun ring_fun = {
    .destroy = ring_destroy,
	.push = ring_push,
	.pop = ring_pop,
	.pop_nocpy = ring_pop_nocpy,
};
// 构造函数实现
Ring* ring_create(uint32_t capacity, uint32_t size) {
    Ring* obj = (Ring*)os_malloc(sizeof(Ring));
    if (obj) {
        memset(obj, 0, sizeof(Ring));
        ring_init(obj, capacity, size);
    }
    return obj;
}

void ring_init(Ring* self, uint32_t capacity, uint32_t size) {
    self->fun = &(ring_fun);
    // TODO: 初始化数据成员
    self->elem_size = size;
    self->capacity = capacity;
    self->head = 0;
    self->tail = 0;
    self->count = 0;
    self->buffer = os_malloc(self->capacity * self->elem_size);
    if (!self->buffer) {
        os_free(self);
        return;
    }
}

void ring_deinit(Ring* self) {
    // TODO: 数据成员申请资源释放
    if (self->buffer) {
        os_free(self->buffer);
    }
}

// 析构函数实现
static void ring_destroy(Ring* self) {
    if (self != NULL) {
        ring_deinit(self);
        os_free(self);
    }
}

// push method
static bool ring_push(Ring* self, const void *data) {
    if (self->count == self->capacity) {
        return false;
    }

    char *byte_buffer = (char *)self->buffer;
    uint32_t key = arch_irq_lock();
    memcpy(byte_buffer + self->tail * self->elem_size, data, self->elem_size);
    self->tail = (self->tail + 1) % self->capacity;
    self->count++;
    arch_irq_unlock(key);
    return true;
}
// pop method
static bool ring_pop(Ring* self, void *data) {
    if (self->count == 0) {
        return false;
    }

    char *byte_buffer = (char *)self->buffer;
    memcpy(data, byte_buffer + self->head * self->elem_size, self->elem_size);
    self->head = (self->head + 1) % self->capacity;
    self->count--;
    return true;
}


// pop_nocpy method
static void * ring_pop_nocpy(Ring* self) {
    if (self->count == 0) {
        return NULL;
    }

    char *byte_buffer = ((char *)self->buffer) + self->head * self->elem_size;
    //memcpy(data, byte_buffer + self->head * self->elem_size, self->elem_size);
    self->head = (self->head + 1) % self->capacity;
    self->count--;
    return byte_buffer;
}


#include "msg_queue.h"
#include <stdio.h>
#include "../common/linear_pool.h"

static uint8_t msg_queue_recv(Msg_queue* self, void **msg, uint32_t timeout);

static uint8_t msg_queue_send(Msg_queue* self, void **msg, uint32_t timeout);

// 析构函数声明
static void msg_queue_destroy(Msg_queue* self);

// TODO: 初始化数据成员
static const Msg_queueFun msg_queue_fun = {
    .destroy = msg_queue_destroy,
	.send = msg_queue_send,
	.recv = msg_queue_recv,
};
// 构造函数实现
Msg_queue* msg_queue_create(uint16_t queue_size) {
    Msg_queue* obj = (Msg_queue*)os_malloc(sizeof(Msg_queue));
    if (obj) {
        memset(obj, 0, sizeof(Msg_queue));
        msg_queue_init(obj, queue_size);
    }
    return obj;
}

void msg_queue_init(Msg_queue* self, uint16_t queue_size) {
    self->fun = &(msg_queue_fun);
    self->ringbuf = ring_create(queue_size, sizeof(void *));
    self->room_sem = semaphore_create(queue_size);
    self->data_sem = semaphore_create(0);
    self->lock = mutex_create();
}

void msg_queue_deinit(Msg_queue* self) {
    // TODO: 数据成员申请资源释放
    if (self->ringbuf) {
        self->ringbuf->fun->destroy(self->ringbuf);
    }
}

// 析构函数实现
static void msg_queue_destroy(Msg_queue* self) {
    if (self != NULL) {
        msg_queue_deinit(self);
        os_free(self);
    }
}

// send method
//只存储消息指针，不存数据
static uint8_t msg_queue_send(Msg_queue* self, void **msg, uint32_t timeout) {
    // 1. 等待空房间（如果队列满，此处会阻塞）
    self->room_sem->fun->take(self->room_sem);
    // 2. 获取互斥锁，操作环形缓冲区
    self->lock->fun->mutex_lock(self->lock, 0);
    self->ringbuf->fun->push(self->ringbuf, *msg);
    self->lock->fun->mutex_unlock(self->lock);
    // 3. 通知数据可用
    self->data_sem->fun->give(self->data_sem);
    return 1;
}


// 只取消息指针，不取数据 recv method
static uint8_t msg_queue_recv(Msg_queue* self, void **msg, uint32_t timeout) {
    // 1. 等待数据可用（如果队列空，此处会阻塞）
    self->data_sem->fun->take(self->data_sem);
    // 2. 获取互斥锁，操作环形缓冲区取出消息
    self->lock->fun->mutex_lock(self->lock, 0);
    self->ringbuf->fun->pop(self->ringbuf, *msg);// 通过输出参数返回消息指针
    self->lock->fun->mutex_unlock(self->lock);
    // 3. 释放一个房间（通知发送者队列有空位）
    self->room_sem->fun->give(self->room_sem);
    return 1;
}


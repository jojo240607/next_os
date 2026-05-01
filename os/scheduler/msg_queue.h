#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "semaphore.h"
#include "mutex.h"
#include "../common/ring.h"

#define GET_MSG_QUEUE(obj) ((Msg_queue *)obj)
// 类声明
typedef struct _Msg_queue Msg_queue;
typedef struct _Msg_queueFun Msg_queueFun;
// 类成员函数结构
struct _Msg_queueFun {
    void (*destroy)(Msg_queue* self);
	uint8_t (*send)(Msg_queue* self, void **msg, uint32_t timeout);

	uint8_t (*recv)(Msg_queue* self, void **msg, uint32_t timeout);

};
// 类结构
struct _Msg_queue {
    const Msg_queueFun* fun;
    // TODO: 添加数据成员
    Ring *ringbuf;
    //void **start;          // 环形缓冲区起始地址
    //uint16_t msg_size;
    //uint16_t queue_len;
    //uint16_t in;
    //uint16_t out;

    Mutex *lock;      // 保护结构体成员的互斥量
    Semaphore *room_sem;    // 计数信号量：初始值 = queue_len（空房间）
    Semaphore *data_sem;    // 计数信号量：初始值 = 0（已有数据）
};

// 构造函数声明
Msg_queue* msg_queue_create(uint16_t queue_size);
void msg_queue_init(Msg_queue* self, uint16_t queue_size);

// 析构函数声明
void msg_queue_deinit(Msg_queue* self);

#endif // MSG_QUEUE_H
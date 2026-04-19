#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "queue.h"


#define GET_SEMAPHORE(obj) ((Semaphore *)obj)
// 类声明
typedef struct _Semaphore Semaphore;
typedef struct _SemaphoreFun SemaphoreFun;
// 类成员函数结构
struct _SemaphoreFun {
    void (*destroy)(Semaphore* self);
	void (*take)(Semaphore* self);
	void (*give)(Semaphore* self);

};
// 类结构
struct _Semaphore {
    const SemaphoreFun* fun;
    // TODO: 添加数据成员
    uint8_t count;                     // 计数值
    Queue *wait_list;              // 等待队列头（单向链表或双向）
};

// 构造函数声明
Semaphore* semaphore_create(uint8_t count);
void semaphore_init(Semaphore* self, uint8_t count);

// 析构函数声明
void semaphore_deinit(Semaphore* self);

#endif // SEMAPHORE_H
#ifndef MUTEX_H
#define MUTEX_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tcb_t.h"

#define GET_MUTEX(obj) ((Mutex *)obj)
// 类声明
typedef struct _Mutex Mutex;
typedef struct _MutexFun MutexFun;
// 类成员函数结构
struct _MutexFun {
    void (*destroy)(Mutex* self);
	uint8_t (*mutex_lock)(Mutex* self, uint32_t timeout_ms);
	uint8_t (*mutex_unlock)(Mutex* self);

};
// 类结构
struct _Mutex {
    const MutexFun* fun;
    // TODO: 添加数据成员
    volatile Tcb_t *owner;       // 当前持有互斥量的任务（NULL 表示空闲）
    volatile uint8_t lock_count;          // 递归锁计数（同一个任务可多次 take）
    // 优先级继承相关
    uint8_t owner_original_prio; // 任务原本的优先级（当继承时记录）
    // 等待队列
    Queue *wait_list;   // 因获取不到互斥量而阻塞的任务（按优先级排序）
};

// 构造函数声明
Mutex* mutex_create();
void mutex_init(Mutex* self);

// 析构函数声明
void mutex_deinit(Mutex* self);

#endif // MUTEX_H
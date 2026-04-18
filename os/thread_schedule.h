#ifndef THREAD_SCHEDULE_H
#define THREAD_SCHEDULE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define GET_THREAD_SCHEDULE(obj) ((Thread_schedule *)obj)
// 类声明
typedef struct _Thread_schedule Thread_schedule;
typedef struct _Thread_scheduleFun Thread_scheduleFun;
// 类成员函数结构
struct _Thread_scheduleFun {
    void (*destroy)(Thread_schedule* self);
};
// 类结构
struct _Thread_schedule {
    const Thread_scheduleFun* fun;
    // TODO: 添加数据成员

};

// 构造函数声明
Thread_schedule* thread_schedule_create();
void thread_schedule_init(Thread_schedule* self);

// 析构函数声明
void thread_schedule_deinit(Thread_schedule* self);

#endif // THREAD_SCHEDULE_H
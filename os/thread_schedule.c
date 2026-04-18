#include "thread_schedule.h"
#include <stdio.h>

// 析构函数声明
static void thread_schedule_destroy(Thread_schedule* self);

// TODO: 初始化数据成员
static const Thread_scheduleFun thread_schedule_fun = {
    .destroy = thread_schedule_destroy,
};
// 构造函数实现
Thread_schedule* thread_schedule_create() {
    Thread_schedule* obj = (Thread_schedule*)malloc(sizeof(Thread_schedule));
    if (obj) {
        memset(obj, 0, sizeof(Thread_schedule));
        thread_schedule_init(obj);
    }
    return obj;
}

void thread_schedule_init(Thread_schedule* self) {
    self->fun = &(thread_schedule_fun);
    // TODO: 初始化数据成员

}

void thread_schedule_deinit(Thread_schedule* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void thread_schedule_destroy(Thread_schedule* self) {
    if (self != NULL) {
        thread_schedule_deinit(self);
        free(self);
    }
}

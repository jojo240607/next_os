#include "time_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"

task_init_override(time_task_task_init_impl);
task_thread_override(time_task_task_thread_impl);

// 析构函数声明
static void time_task_destroy(Time_task* self);

// TODO: 初始化数据成员
static const Time_taskFun time_task_fun = {
    .destroy = time_task_destroy,
};
// 构造函数实现
Time_task* time_task_create() {
    Time_task* obj = (Time_task*)os_malloc(sizeof(Time_task));
    if (obj) {
        memset(obj, 0, sizeof(Time_task));
        time_task_init(obj);
    }
    return obj;
}

void time_task_init(Time_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(time_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = time_task_task_init_impl;
	def_task_thread(self) = time_task_task_thread_impl;
    self->timer = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_TIME2);
}

void time_task_deinit(Time_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void time_task_destroy(Time_task* self) {
    if (self != NULL) {
        time_task_deinit(self);
        os_free(self);
    }
}

// task_init method
task_init_override(time_task_task_init_impl) {
    // TODO: add task_init method
    Time_task *time_task = (Time_task *)self;
    //params , void *parent
    //params , void *parent
    if (!time_task) {
        return;
    }
    virtual_dev_init(time_task->timer, self->task_tcb->semaphore);
}
// task_thread method
task_thread_override(time_task_task_thread_impl) {
    // TODO: add task_thread method
    //Time_task *time_task = (Time_task *)self;
    //params , void *arg
    while (true) {
        self->semaphore->fun->take(self->semaphore);
        LOG_DEBUG("time_task", "----- timer on -----");

    }
}


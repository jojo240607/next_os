#include "bus_task.h"
#include <stdio.h>
#include <stdlib.h>

task_init_override(bus_task_task_init_impl);
task_thread_override(bus_task_task_thread_impl);

// 析构函数声明
static void bus_task_destroy(Bus_task* self);

// TODO: 初始化数据成员
static const Bus_taskFun bus_task_fun = {
    .destroy = bus_task_destroy,
};
// 构造函数实现
Bus_task* bus_task_create() {
    Bus_task* obj = (Bus_task*)malloc(sizeof(Bus_task));
    if (obj) {
        memset(obj, 0, sizeof(Bus_task));
        bus_task_init(obj);
    }
    return obj;
}

void bus_task_init(Bus_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(bus_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = bus_task_task_init_impl;
	def_task_thread(self) = bus_task_task_thread_impl;
    self->bus = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_BUS);
}

void bus_task_deinit(Bus_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void bus_task_destroy(Bus_task* self) {
    if (self != NULL) {
        bus_task_deinit(self);
        free(self);
    }
}

// task_init method
task_init_override(bus_task_task_init_impl) {
    // TODO: add task_init method
    Bus_task *bus_task = (Bus_task *)self;
    //params , void *parent
    //params , void *parent
    if (!bus_task) {
        return;
    }
    virtual_dev_init(bus_task->bus, self->task_tcb->semaphore);
}
// task_thread method
task_thread_override(bus_task_task_thread_impl) {
    // TODO: add task_thread method
    Bus_task *bus_task = (Bus_task *)self;
    //params , void *arg
    while (1) {
        self->semaphore->fun->take(self->semaphore);
    }
}


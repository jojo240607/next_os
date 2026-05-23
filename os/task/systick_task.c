#include "systick_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"

task_init_override(systick_task_task_init_impl);

// 析构函数声明
static void systick_task_destroy(Systick_task* self);

// TODO: 初始化数据成员
static const Systick_taskFun systick_task_fun = {
    .destroy = systick_task_destroy,
};
// 构造函数实现
Systick_task* systick_task_create() {
    Systick_task* obj = (Systick_task*)os_malloc(sizeof(Systick_task));
    if (obj) {
        memset(obj, 0, sizeof(Systick_task));
        systick_task_init(obj);
    }
    return obj;
}

void systick_task_init(Systick_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(systick_task_fun);
    // TODO: 初始化派生类特有成员
    self->systick = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_SYSTICK);
	def_task_init(self) = systick_task_task_init_impl;
}

void systick_task_deinit(Systick_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
    if (self->systick) {
        self->systick->fun->destroy(self->systick);
    }
}
// 析构函数实现
static void systick_task_destroy(Systick_task* self) {
    if (self != NULL) {
        systick_task_deinit(self);
        os_free(self);
    }
}

// task_init method
task_init_override(systick_task_task_init_impl) {
    // TODO: add task_init method
    LOG_DEBUG("systick","systick_task init");
    Systick_task *systick_task = (Systick_task *)self;
    //params , void *parent
    virtual_dev_init(systick_task->systick, NULL);
    GET_SYSTICK(systick_task->systick)->fun->start(GET_SYSTICK(systick_task->systick));
}


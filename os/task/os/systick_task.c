#include "systick_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../../driver/device_manager.h"
task_start_override(systick_task_task_start_impl);

task_init_override(systick_task_task_init_impl);

// 析构函数声明
static void systick_task_destroy(Systick_task* self);

// TODO: 初始化数据成员
static const Systick_taskFun systick_task_fun = {
    .destroy = systick_task_destroy,
};
// 构造函数实现
Systick_task* systick_task_create(const task_into_t *info) {
    Systick_task* obj = (Systick_task*)os_malloc(sizeof(Systick_task));
    if (obj) {
        memset(obj, 0, sizeof(Systick_task));
        systick_task_init(obj, info);
    }
    return obj;
}

void systick_task_init(Systick_task* self, const task_into_t *info) {
    // 初始化基类部分
    task_init(&self->base, info);
    self->fun = &(systick_task_fun);
    // TODO: 初始化派生类特有成员
    self->systick = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_SYSTICK);
    /* 注意：systick 任务直接通过 device_manager 打开设备，与任务配置耦合。
     * 重构时可考虑将设备依赖注入到 task_into_t 中。 */
	def_task_init(self) = systick_task_task_init_impl;
	def_task_start(self) = systick_task_task_start_impl;
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

}


// task_start method
task_start_override(systick_task_task_start_impl) {
    // TODO: add task_start method
    Systick_task *systick_task = (Systick_task *)self;
    //params 
    systick_task->systick->vtable->dev_ioctl(systick_task->systick, DEVICE_START, NULL);
}


#include "monitor_task.h"
#include "task_manager.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>

static void monitor_task_cpu_usage(Monitor_task* self);

task_init_override(monitor_task_task_init_impl);
task_thread_override(monitor_task_task_thread_impl);

// 析构函数声明
static void monitor_task_destroy(Monitor_task* self);

// TODO: 初始化数据成员
static const Monitor_taskFun monitor_task_fun = {
    .destroy = monitor_task_destroy,
	.cpu_usage = monitor_task_cpu_usage,
};
// 构造函数实现
Monitor_task* monitor_task_create() {
    Monitor_task* obj = (Monitor_task*)os_malloc(sizeof(Monitor_task));
    if (obj) {
        memset(obj, 0, sizeof(Monitor_task));
        monitor_task_init(obj);
    }
    return obj;
}

void monitor_task_init(Monitor_task* self) {
    // 初始化基类部分
    base_task_init(&self->base);
    self->fun = &(monitor_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = monitor_task_task_init_impl;
	def_task_thread(self) = monitor_task_task_thread_impl;
    self->idletask = NULL;
    self->last_call = 0;
    self->cpu_usage = 0;
}

void monitor_task_deinit(Monitor_task* self) {
    base_task_deinit(GET_BASE_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void monitor_task_destroy(Monitor_task* self) {
    if (self != NULL) {
        monitor_task_deinit(self);
        os_free(self);
    }
}

// task_init method
task_init_override(monitor_task_task_init_impl) {
    // TODO: add task_init method
    Monitor_task *monitor_task = (Monitor_task *)self;
    //params 
    monitor_task->idletask = GET_IDLE_TASK(GET_TASK_MANAGER(parent)->task_tab[TASK_IDLE]);
}
// task_thread method
task_thread_override(monitor_task_task_thread_impl) {
    // TODO: add task_thread method
    Monitor_task *monitor_task = (Monitor_task *)self->parent;
    //params , void *arg
    while(true) {
        self->fun->os_sleep(self, 3000);
        monitor_task_cpu_usage(monitor_task);
    }
}


// cpu_usage method
static void monitor_task_cpu_usage(Monitor_task* self) {
    if (NULL == self) {
        return;
    }

    uint32_t now_call = HAL_GetTick();//get_system_tick();
    uint32_t run_time =  now_call - self->last_call;
    self->last_call = now_call;
    // 假设每秒统计一次，总时间片为 SYSTEM_TICKS_PER_SEC
    self->cpu_usage = 100 - (self->idletask->idle_total_ticks * 100 / run_time);
    self->idletask->idle_total_ticks = 0;
    
}


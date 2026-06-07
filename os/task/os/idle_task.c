#include "idle_task.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include <stdio.h>
#include <stdlib.h>

task_init_override(idle_task_init_impl);
task_thread_override(idle_task_thread_impl);

// 析构函数声明
static void idle_task_destroy(Idle_task* self);

// TODO: 初始化数据成员
static const Idle_taskFun idle_task_fun = {
    .destroy = idle_task_destroy,
};
// 构造函数实现
Idle_task* idle_task_create(const task_into_t *info) {
    Idle_task* obj = (Idle_task*)os_malloc(sizeof(Idle_task));
    if (obj) {
        memset(obj, 0, sizeof(Idle_task));
        idle_task_init(obj, info);
    }
    return obj;
}

void idle_task_init(Idle_task* self, const task_into_t *info) {
    task_init(&self->base, info);
    self->fun = &(idle_task_fun);
	def_task_init(self) = idle_task_init_impl;
	def_task_thread(self) = idle_task_thread_impl;
}

void idle_task_deinit(Idle_task* self) {
    task_deinit(GET_TASK(self));
}
// 析构函数实现
static void idle_task_destroy(Idle_task* self) {
    if (self != NULL) {
        idle_task_deinit(self);
        os_free(self);
    }
}
// init method
task_init_override(idle_task_init_impl) {
    // TODO: add init method
    LOG_DEBUG("idle","idle_task init");
    //Idle_task *idle_task = (Idle_task *)self;
    //params
}
// thread method
task_thread_override(idle_task_thread_impl) {
    // TODO: add thread method
    //Idle_task *idle_task = (Idle_task *)self->parent;
    //uint32_t last_idle_start = 0;
    //params , void *arg
    while (true) {
        while (global_thread_scheduler->destroy_list->size) {
            Tcb_t *destroy_tcb = GET_TCB_T(global_thread_scheduler->destroy_list->fun->dequeue(global_thread_scheduler->destroy_list));
            if (destroy_tcb) {
                destroy_tcb->fun->destroy(destroy_tcb);
            }
        }
        //LOG_DEBUG(MODULE_SYSTEM,"idle\n");
        LOW_POWER;//进入低功耗模式
    }
}


#include "idle_task.h"
#include "../common/linear_pool.h"
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
Idle_task* idle_task_create() {
    Idle_task* obj = (Idle_task*)os_malloc(sizeof(Idle_task));
    if (obj) {
        memset(obj, 0, sizeof(Idle_task));
        idle_task_init(obj);
    }
    return obj;
}

void idle_task_init(Idle_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(idle_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = idle_task_init_impl;
	def_task_thread(self) = idle_task_thread_impl;
    self->idle_total_ticks = 0;
}

void idle_task_deinit(Idle_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
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
    Idle_task *idle_task = (Idle_task *)self;
    //params
}
// thread method
task_thread_override(idle_task_thread_impl) {
    // TODO: add thread method
    Idle_task *idle_task = (Idle_task *)self->parent;
    //uint32_t last_idle_start = 0;
    //params , void *arg
    while (true) {
        while (global_thread_scheduler->destory_list->size) {
            Tcb_t *destory_tcb = GET_TCB_T(global_thread_scheduler->destory_list->fun->dequeue(global_thread_scheduler->destory_list));
            if (destory_tcb) {
                destory_tcb->fun->destroy(destory_tcb);
            }
        }
        LOW_POWER;//进入低功耗模式
        idle_task->idle_total_ticks += self->run_time;
    }
}


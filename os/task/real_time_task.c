#include "real_time_task.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>

static void real_time_task_quick_task(Real_time_task* self);

task_thread_override(real_time_task_thread_impl);

// 析构函数声明
static void real_time_task_destroy(Real_time_task* self);

// TODO: 初始化数据成员
static const Real_time_taskFun real_time_task_fun = {
    .destroy = real_time_task_destroy,
	.quick_task = real_time_task_quick_task,
};
// 构造函数实现
Real_time_task* real_time_task_create(const char *name) {
    Real_time_task* obj = (Real_time_task*)os_malloc(sizeof(Real_time_task));
    if (obj) {
        memset(obj, 0, sizeof(Real_time_task));
        real_time_task_init(obj, name);
    }
    return obj;
}

void real_time_task_init(Real_time_task* self, const char *name) {
    if (GET_REAL_TIME_TASK_VTABLE(self) == NULL) {
        GET_REAL_TIME_TASK_VTABLE(self) = (Real_time_taskVTable *) malloc(sizeof(Real_time_taskVTable));
        memset(GET_REAL_TIME_TASK_VTABLE(self), 0, sizeof(Real_time_taskVTable));
    }
    // 初始化基类部分
    base_task_init(&self->base);
    self->fun = &(real_time_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_thread(self) = real_time_task_thread_impl;
}

void real_time_task_deinit(Real_time_task* self) {
    if (GET_REAL_TIME_TASK_VTABLE(self) != NULL) {
        os_free(GET_REAL_TIME_TASK_VTABLE(self));
        GET_REAL_TIME_TASK_VTABLE(self) = NULL;
    }
    base_task_deinit(GET_BASE_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void real_time_task_destroy(Real_time_task* self) {
    if (self != NULL) {
        real_time_task_deinit(self);
        os_free(self);
    }
}

// thread method
task_thread_override(real_time_task_thread_impl) {
    //params , Tcb_t* self, void *arg
    if (def_button_half_task(self->parent) == NULL) {//未定义下半部函数，则直接退出
        return;
    }
    while (true) {
        self->semaphore->fun->take(self->semaphore);//take变成阻塞状态
        virtual_button_half_task(self->parent, arg);//下半部函数
    }
}
// quick_task method
static void real_time_task_quick_task(Real_time_task* self) {
    // TODO: add quick_task method
    if (def_top_half_task(self) == NULL) {//未定义上半部函数，则直接退出
        return;
    }
    if (def_button_half_task(self) != NULL) {
        virtual_top_half_task(self);
        GET_BASE_TASK(self)->task_tcb->semaphore->fun->give(GET_BASE_TASK(self)->task_tcb->semaphore);//give 让任务运行起来
    }
}


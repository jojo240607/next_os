#include "base_task.h"
#include "../common/linear_pool.h"
#include <stdio.h>

static void base_task_add_task(Base_task* self, const char *name, uint8_t priority);

// 析构函数声明
static void base_task_destroy(Base_task* self);

// TODO: 初始化数据成员
static const Base_taskFun base_task_fun = {
    .destroy = base_task_destroy,
	.add_task = base_task_add_task,
};
// 构造函数实现
Base_task* base_task_create() {
    Base_task* obj = (Base_task*)os_malloc(sizeof(Base_task));
    if (obj) {
        memset(obj, 0, sizeof(Base_task));
        base_task_init(obj);
    }
    return obj;
}

void base_task_init(Base_task* self) {
    if (self->vtable == NULL) {
        self->vtable = (Base_taskVTable *) malloc(sizeof(Base_taskVTable));
        memset(self->vtable , 0, sizeof(Base_taskVTable));
    }
    self->fun = &(base_task_fun);
    // TODO: 初始化数据成员
    self->task_tcb = NULL;
}

void base_task_deinit(Base_task* self) {
    if (self->vtable != NULL) {
        os_free(self->vtable);
        self->vtable = NULL;
    }
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void base_task_destroy(Base_task* self) {
    if (self != NULL) {
        base_task_deinit(self);
        os_free(self);
    }
}

// add_task method
static void base_task_add_task(Base_task* self, const char *name, uint8_t priority) {
    if (NULL == self) {
        return;
    }
    if (def_task_thread(self) != NULL) {
        Entry_t entry_s = {
                .priority = priority,
                .parent = self,
                .entry_fun = def_task_thread(self),
                .arg = NULL//arg need transfer to thread
        };
        self->task_tcb = global_thread_scheduler->fun->create_thread(global_thread_scheduler, name, &entry_s);
    }
}


#include "task.h"
#include "../common/linear_pool.h"
#include <stdio.h>

static void task_add_task(Task* self, const char *name, uint8_t priority, size_t stack_size);

// 析构函数声明
static void task_destroy(Task* self);

// TODO: 初始化数据成员
static const TaskFun Task_fun = {
    .destroy = task_destroy,
	.add_task = task_add_task,
};
// 构造函数实现
Task* Task_create() {
    Task* obj = (Task*)os_malloc(sizeof(Task));
    if (obj) {
        memset(obj, 0, sizeof(Task));
        task_init(obj);
    }
    return obj;
}

void task_init(Task* self) {
    if (self->vtable == NULL) {
        self->vtable = (TaskVTable *) os_malloc(sizeof(TaskVTable));
        memset(self->vtable , 0, sizeof(TaskVTable));
    }
    self->fun = &(Task_fun);
    // TODO: 初始化数据成员
    self->task_tcb = NULL;
}

void task_deinit(Task* self) {
    if (self->vtable != NULL) {
        os_free(self->vtable);
        self->vtable = NULL;
    }
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void task_destroy(Task* self) {
    if (self != NULL) {
        task_deinit(self);
        os_free(self);
    }
}

// add_task method
static void task_add_task(Task* self, const char *name, uint8_t priority, size_t stack_size) {
    if (NULL == self) {
        return;
    }
    if (GET_TASK_VTABLE(self)->task_thread != NULL && stack_size > 0) {
        Entry_t entry_s = {
                .priority = priority,
                .stack_size = stack_size,
                .parent = self,
                .entry_fun = def_task_thread(self),
                .arg = NULL//arg need transfer to thread
        };
        self->task_tcb = global_thread_scheduler->fun->create_thread(global_thread_scheduler, name, &entry_s);
    }
}


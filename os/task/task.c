#include "task.h"
#include "../common/linear_pool.h"
#include "../driver/common/nvic.h"
#include <stdio.h>

static void * task_get_event(Task* self);
static void task_trigger(Task* self, void *event);
static void task_add_thread(Task* self);

// 析构函数声明
static void task_destroy(Task* self);

// TODO: 初始化数据成员
static const TaskFun Task_fun = {
    .destroy = task_destroy,
	.add_thread = task_add_thread,
    .trigger = task_trigger,
    .get_event = task_get_event,
};
// 构造函数实现
Task* task_create(const task_into_t *info) {
    Task* obj = (Task*)os_malloc(sizeof(Task));
    if (obj) {
        memset(obj, 0, sizeof(Task));
        task_init(obj, info);
    }
    return obj;
}

void task_init(Task* self, const task_into_t *info) {
    if (self->vtable == NULL) {
        self->vtable = (TaskVTable *) os_malloc(sizeof(TaskVTable));
        memset(self->vtable , 0, sizeof(TaskVTable));
    }
    self->fun = &(Task_fun);
    // TODO: 初始化数据成员
    self->task_tcb = NULL;
    self->info = info;
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
static void task_add_thread(Task* self) {
    if (NULL == self) {
        return;
    }
    if (GET_TASK_VTABLE(self)->task_thread != NULL && self->info->stack_size > MPU_SIZE_0B) {
        thread_conf_t conf = {
                .priority = self->info->priority,
                .stack_size = self->info->stack_size,
                .parent = self,
                .loop = def_task_thread(self),
                .arg = NULL//arg need transfer to thread
        };
        self->task_tcb = global_thread_scheduler->fun->create_thread(global_thread_scheduler, self->info->name, &conf);
    }
}


// trigger method
static void task_trigger(Task* self, void *event) {
    // TODO: add trigger method
    if (self->task_tcb && self->task_tcb->semaphore) {
        self->task_tcb->semaphore->sem_event = (uint32_t)event;
        self->task_tcb->semaphore->fun->give(self->task_tcb->semaphore);
    }
}


// get_event method
static void * task_get_event(Task* self) {
    // TODO: add get_event method
    if (self->task_tcb && self->task_tcb->semaphore) {
        return (void *)self->task_tcb->semaphore->sem_event;
    }
    return NULL;
}


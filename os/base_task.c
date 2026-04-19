#include "base_task.h"
#include <stdio.h>

// 析构函数声明
static void base_task_destroy(Base_task* self);

// TODO: 初始化数据成员
static const Base_taskFun base_task_fun = {
    .destroy = base_task_destroy,
};
// 构造函数实现
Base_task* base_task_create(const char *name) {
    Base_task* obj = (Base_task*)malloc(sizeof(Base_task));
    if (obj) {
        memset(obj, 0, sizeof(Base_task));
        base_task_init(obj, name);
    }
    return obj;
}

void base_task_init(Base_task* self, const char *name) {
    if (self->vtable == NULL) {
        self->vtable = (Base_taskVTable *) malloc(sizeof(Base_taskVTable));
        memset(self->vtable , 0, sizeof(Base_taskVTable));
    }
    self->fun = &(base_task_fun);
    // TODO: 初始化数据成员
    self->task_tcb = tcb_t_create();
    gloable_thread_schedule->fun->create_thread(gloable_thread_schedule, name, &def_thread(self));
}

void base_task_deinit(Base_task* self) {
    if (self->vtable != NULL) {
        free(self->vtable);
        self->vtable = NULL;
    }
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void base_task_destroy(Base_task* self) {
    if (self != NULL) {
        base_task_deinit(self);
        free(self);
    }
}

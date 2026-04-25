#include "task_test1.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>

task_init_override(task_test1_init_impl);
task_thread_override(task_test1_thread_impl);

// 析构函数声明
static void task_test1_destroy(Task_test1* self);

// TODO: 初始化数据成员
static const Task_test1Fun task_test1_fun = {
    .destroy = task_test1_destroy,
};
// 构造函数实现
Task_test1* task_test1_create(const char *name, uint8_t priority) {
    Task_test1* obj = (Task_test1*)os_malloc(sizeof(Task_test1));
    if (obj) {
        memset(obj, 0, sizeof(Task_test1));
        task_test1_init(obj, name, priority);
    }
    return obj;
}

void task_test1_init(Task_test1* self, const char *name, uint8_t priority) {
    // 初始化基类部分
    base_task_init(&self->base);
    self->fun = &(task_test1_fun);
    // TODO: 初始化派生类特有成员
	def_task_init(self) = task_test1_init_impl;
	def_task_thread(self) = task_test1_thread_impl;
}

void task_test1_deinit(Task_test1* self) {
    base_task_deinit(GET_BASE_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void task_test1_destroy(Task_test1* self) {
    if (self != NULL) {
        task_test1_deinit(self);
        os_free(self);
    }
}

// init method
task_init_override(task_test1_init_impl) {
    // TODO: add init method
    Task_test1 *task_test1 = (Task_test1 *)self;
    //params 
    
}
// thread method
task_thread_override(task_test1_thread_impl) {
    // TODO: add thread method
    Task_test1 *task_test1 = (Task_test1 *)self;
    //params , void *arg
    while (true) {
        for(uint32_t i = 0; i < 100; i++) {
            for(uint32_t j = 0; j < 100; j++) {
                printf("idle_task\n");
            }
        }
        self->fun->os_sleep(self, 50);
    }
}


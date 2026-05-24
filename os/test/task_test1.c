#include "task_test1.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <fastmath.h>
#include "../log/log.h"
#include "../common/sys_mutex.h"

task_init_override(task_test1_init_impl);
task_thread_override(task_test1_thread_impl);

// 析构函数声明
static void task_test1_destroy(Task_test1* self);
static void task_test1_callback_handler(os_cb_event event, void *arg);

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
    task_init(&self->base);
    self->fun = &(task_test1_fun);
    // TODO: 初始化派生类特有成员
	def_task_init(self) = task_test1_init_impl;
	def_task_thread(self) = task_test1_thread_impl;
    //self->exti = GET_EXTI(gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_EXTI));

    self->callback.cb_handler = task_test1_callback_handler;
    self->callback.cb_event = OS_EVENT_EXTI1;//0000 0010
    self->callback.arg = self;
    self->callback.base.next = NULL;
   // self->exti->fun->register_exti_cb(self->exti, &self->callback);
    self->start_mutex = false;
    self->locked = false;
    register_callback(&self->callback);
}

void task_test1_deinit(Task_test1* self) {
    task_deinit(GET_TASK(self));
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
    LOG_DEBUG("test1", "task_test1 init");
    Task_test1 *task_test1 = (Task_test1 *)self;
    //params 
    //virtual_dev_init(GET_DEVICE(task_test1->exti), NULL);
}
// thread method
task_thread_override(task_test1_thread_impl) {
    // TODO: add thread method
    Task_test1 *task_test1 = (Task_test1 *)self->parent;
    //params , void *arg
    float dd = 1.0f;
    float a = 1.2345f, b = 6.7890f, result;
    volatile float f_result; // volatile 避免优化掉
    f_result = a + b;
    f_result = a * b;
    f_result = a / b;
    f_result = sqrtf(a);
    f_result = a * b + 1.0f;
    while (true) {
        for (uint32_t i = 0; i < 100; i++) {
            for(uint32_t j = 0; j < 100; j++) {
                dd *= 3.14f;
                f_result = a + b;
                f_result = a * b;
                f_result = a / b;
                f_result = sqrtf(a);
                f_result = a * b + 1.0f;
            }
        }
        LOG_DEBUG("test1", "dd = %x", dd);
        self->fun->os_sleep(self, 300);
        if (task_test1->start_mutex) {
            task_test1->start_mutex = false;
            if (!task_test1->locked) {
                LOG_DEBUG("test1", "start_mutex lock -----------------");
                if (gloable_mutex->fun->mutex_lock(gloable_mutex, 0)) {
                    task_test1->locked = true;
                } else {
                    LOG_DEBUG("test1", "start_mutex lock fail-----------------");
                }
            } else {
                LOG_DEBUG("test1", "start_mutex unlock -----------------");
                if (gloable_mutex->fun->mutex_unlock(gloable_mutex)) {
                    task_test1->locked = false;
                }
            }
        }
        LOG_DEBUG("test1", "task_test1");
    }
}

static void task_test1_callback_handler(os_cb_event event, void *arg) {
    Task_test1 *task_test1 = (Task_test1 *)arg;
    //LOG_DEBUG("test1", "-------- exti %d task %s --------", event, GET_TASK(task_test1)->task_tcb->name);
    task_test1->start_mutex = true;
}

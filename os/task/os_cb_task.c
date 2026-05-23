#include "os_cb_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"


task_init_override(os_cb_task_task_init_impl);
task_thread_override(os_cb_task_task_thread_impl);

// 析构函数声明
static void os_cb_task_destroy(Os_cb_task* self);

// TODO: 初始化数据成员
static const Os_cb_taskFun os_cb_task_fun = {
    .destroy = os_cb_task_destroy,
};
static Os_cb_task *gloable_os_callback;
// 构造函数实现
Os_cb_task* os_cb_task_create() {
    Os_cb_task* obj = (Os_cb_task*)os_malloc(sizeof(Os_cb_task));
    if (obj) {
        memset(obj, 0, sizeof(Os_cb_task));
        os_cb_task_init(obj);
    }
    gloable_os_callback = obj;
    return obj;
}

void os_cb_task_init(Os_cb_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(os_cb_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = os_cb_task_task_init_impl;
	def_task_thread(self) = os_cb_task_task_thread_impl;
    self->cb_event = 0;
    self->callback_list = queue_create();
    self->exti = GET_EXTI(gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_EXTI));
}

void os_cb_task_deinit(Os_cb_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
    if (self->callback_list) {
        self->callback_list->fun->destroy(self->callback_list);
    }
}
// 析构函数实现
static void os_cb_task_destroy(Os_cb_task* self) {
    if (self != NULL) {
        os_cb_task_deinit(self);
        os_free(self);
    }
}
// register_exti_cb method
void register_callback(os_callback *callback) {
    if (!gloable_os_callback) {
        LOG_ERROR("os_cb_task", "gloable_os_callback is null");
        return;
    }
    gloable_os_callback->callback_list->fun->enqueue(gloable_os_callback->callback_list, GET_NODE(callback));

}

// task_init method
task_init_override(os_cb_task_task_init_impl) {
    // TODO: add task_init method
    Os_cb_task *os_cb_task = (Os_cb_task *)self;
    //params , void *parent
    virtual_dev_init(GET_DEVICE(os_cb_task->exti), self->task_tcb->semaphore);
}
// task_thread method
task_thread_override(os_cb_task_task_thread_impl) {
    // TODO: add task_thread method
    Os_cb_task *os_cb_task = (Os_cb_task *)self->parent;

    //params , void *arg
    while (true) {
        self->semaphore->fun->take_user(self->semaphore);
        os_cb_task->cb_event = self->semaphore->sem_event;
        LOG_DEBUG("os_cb_task", "----- os callback ----- event %d", os_cb_task->cb_event);
        os_callback *cb = (os_callback*)os_cb_task->callback_list->head;
        while (cb != NULL && cb->cb_event == os_cb_task->cb_event) {
            LOG_DEBUG("os_cb_task", "do cb_handler");
            ((os_callback*)os_cb_task->callback_list->head)->cb_handler(os_cb_task->cb_event, cb->arg);
            cb = (os_callback *)GET_NODE(cb)->next;
        }
    }
}


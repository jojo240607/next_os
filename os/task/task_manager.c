#include "task_manager.h"
#include <stdio.h>
#include "os/systick_task.h"
#include "os/idle_task.h"
#include "os/monitor_task.h"
#include "os/log_task.h"
#include "os/os_cb_task.h"
#include "../test/time_task.h"
#include "../test/task_test1.h"
#include "../common/linear_pool.h"
#include "task_config.h"

static void task_manager_boot_init(Task_manager* self);

// 析构函数声明
static void task_manager_destroy(Task_manager* self);

// TODO: 初始化数据成员
static const Task_managerFun task_manager_fun = {
    .destroy = task_manager_destroy,
	.boot_init = task_manager_boot_init,
};
Task_manager * gloable_taskManager;
// 构造函数实现
Task_manager* task_manager_create() {
    Task_manager* obj = (Task_manager*)os_malloc(sizeof(Task_manager) + totel_task * sizeof(Task *));
    if (obj) {
        memset(obj, 0, sizeof(Task_manager) + totel_task * sizeof(Task *));
        task_manager_init(obj);
    }
    return obj;
}

void task_manager_init(Task_manager* self) {
    LOG_DEBUG("task_manager","task_manager_init");
    self->fun = &(task_manager_fun);
    // TODO: 初始化数据成员
    self->task_list_ptr = &task_lists;
    self->task_size = totel_task;

}

void task_manager_deinit(Task_manager* self) {
    // TODO: 数据成员申请资源释放
    for (uint8_t i = 0; i < self->task_size; i++) {
        if (self->task_tab[i]) {
            self->task_tab[i]->fun->destroy(self->task_tab[i]);
        }
    }
}

// 析构函数实现
static void task_manager_destroy(Task_manager* self) {
    if (self != NULL) {
        task_manager_deinit(self);
        os_free(self);
    }
}

// tasks_init method
static void task_manager_boot_init(Task_manager* self) {
    LOG_DEBUG("task_manager", "device boot_init");
    if (NULL == self) {
        return;
    }
    uint16_t task_num = 0;
    while (task_num < self->task_size) {
        Task **newtask = self->task_tab + task_num;
        *newtask = ((*self->task_list_ptr)[task_num])->create((*self->task_list_ptr)[task_num]);
        LOG_DEBUG("task_manager", "create thread [%s]", ((*self->task_list_ptr)[task_num])->name);
        (*newtask)->fun->add_thread(*newtask);
        if ((*newtask)->vtable->task_init) {
            (*newtask)->vtable->task_init(*newtask, self);
        }
        task_num++;
    }
    task_num = 0;
    while (task_num < self->task_size) {
        Task **cur_task = self->task_tab + task_num;
        if ((*cur_task)->vtable->task_start) {
            LOG_DEBUG("task_manager", "start thread [%s]", ((*self->task_list_ptr)[task_num])->name);
            (*cur_task)->vtable->task_start(*cur_task);
        }
        task_num++;
    }
}


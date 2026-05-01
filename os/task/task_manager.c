#include "task_manager.h"
#include <stdio.h>
#include "systick_task.h"
#include "idle_task.h"
#include "monitor_task.h"
#include "log_task.h"
#include "os_cb_task.h"
#include "../test/time_task.h"
#include "../test/task_test1.h"
#include "../common/linear_pool.h"

static void task_manager_boot_init(Task_manager* self);

// 析构函数声明
static void task_manager_destroy(Task_manager* self);

// TODO: 初始化数据成员
static const Task_managerFun task_manager_fun = {
    .destroy = task_manager_destroy,
	.boot_init = task_manager_boot_init,
};
Task_manager * gloable_taskManager;
static const Task_list task_lists[] = {
        {.tag = TASK_SYSTICK, .name = "systick", .priority = 1, .stack_size = 0, .task_create = (Task_create) systick_task_create},
        {.tag = TASK_OS_CALLBACK, .name = "os_cb", .priority = 5, .stack_size = 192, .task_create = (Task_create) os_cb_task_create},
        {.tag = TASK_IDLE, .name = "idle", .priority = 0, .stack_size = 32, .task_create = (Task_create) idle_task_create},
        {.tag = TASK_MONITOR, .name = "monitor", .priority = 0, .stack_size = 192, .task_create = (Task_create) monitor_task_create},
        {.tag = TASK_LOG, .name = "log", .priority = 0, .stack_size = 192, .task_create = (Task_create) log_task_create},
        {.tag = TASK_TEST1, .name = "test1", .priority = 1, .stack_size = DEFAULT_STACK_SIZE, .task_create = (Task_create) task_test1_create},
        {.tag = TASK_TIME, .name = "time", .priority = 1, .stack_size = 192, .task_create = (Task_create) time_task_create},

};
// 构造函数实现
Task_manager* task_manager_create() {
    Task_manager* obj = (Task_manager*)os_malloc(sizeof(Task_manager) + ARRAY_SIZE(task_lists) * sizeof(Task *));
    if (obj) {
        memset(obj, 0, sizeof(Task_manager) + ARRAY_SIZE(task_lists) * sizeof(Task *));
        task_manager_init(obj);
    }
    return obj;
}

void task_manager_init(Task_manager* self) {
    LOG_DEBUG("task_manager","task_manager_init");
    self->fun = &(task_manager_fun);
    // TODO: 初始化数据成员
    self->task_list = task_lists;
    self->task_size = ARRAY_SIZE(task_lists);

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
    LOG_DEBUG("task_manager", "task_manager boot_init");
    if (NULL == self) {
        return;
    }
    uint16_t task_num = 0;
    while (task_num < self->task_size) {
        self->task_tab[(self->task_list + task_num)->tag] = (self->task_list + task_num)->task_create();
        LOG_DEBUG("task_manager", "create thread [%s]", (self->task_list + task_num)->name);
        self->task_tab[(self->task_list + task_num)->tag]->fun->add_task(
                self->task_tab[(self->task_list + task_num)->tag],
                (self->task_list + task_num)->name,
                (self->task_list + task_num)->priority,
                (self->task_list + task_num)->stack_size);
        if (def_task_init(self->task_tab[(self->task_list + task_num)->tag])) {
            virtual_task_init(self->task_tab[(self->task_list + task_num)->tag], self);
        }
        task_num++;
    }
}


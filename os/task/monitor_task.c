#include "monitor_task.h"
#include "task_manager.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../common/sys_time.h"
#include "../task/task_manager.h"
#include "../common/sys_mutex.h"


task_init_override(monitor_task_task_init_impl);
task_thread_override(monitor_task_task_thread_impl);

// 析构函数声明
static void monitor_task_destroy(Monitor_task* self);

// TODO: 初始化数据成员
static const Monitor_taskFun monitor_task_fun = {
    .destroy = monitor_task_destroy,
};
// 构造函数实现
Monitor_task* monitor_task_create() {
    Monitor_task* obj = (Monitor_task*)os_malloc(sizeof(Monitor_task));
    if (obj) {
        memset(obj, 0, sizeof(Monitor_task));
        monitor_task_init(obj);
    }
    return obj;
}

void monitor_task_init(Monitor_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(monitor_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = monitor_task_task_init_impl;
	def_task_thread(self) = monitor_task_task_thread_impl;
    self->idletask = NULL;
    self->last_call = 0;
}

void monitor_task_deinit(Monitor_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void monitor_task_destroy(Monitor_task* self) {
    if (self != NULL) {
        monitor_task_deinit(self);
        os_free(self);
    }
}

// task_init method
task_init_override(monitor_task_task_init_impl) {
    // TODO: add task_init method
    LOG_DEBUG("monitor","monitor_task init");
    Monitor_task *monitor_task = (Monitor_task *)self;
    //params 
    monitor_task->idletask = GET_IDLE_TASK(GET_TASK_MANAGER(parent)->task_tab[TASK_IDLE]);
}
// task_thread method
task_thread_override(monitor_task_task_thread_impl) {
    // TODO: add task_thread method
    Monitor_task *monitor_task = (Monitor_task *)self->parent;
    //params , void *arg
    while(true) {
        LOG_DEBUG("monitor", "----------------monitor lock----------------");
        gloable_mutex->fun->mutex_lock(gloable_mutex, 0);
        self->fun->os_sleep(self, 3000);
        uint32_t now = get_systime_us();
        uint32_t run_time =  now - monitor_task->last_call;
        monitor_task->last_call = now;
        LOG_DEBUG("monitor", "--------------------------------");
        LOG_DEBUG("monitor", "name      cpu     mem     stack");
        for (uint8_t i =0; i < gloable_taskManager->task_size; i++) {
            if (gloable_taskManager->task_tab[i]->task_tcb) {
                gloable_taskManager->task_tab[i]->task_tcb->cpu_usage_info.usage_percent =
                        (gloable_taskManager->task_tab[i]->task_tcb->cpu_usage_info.total_run_time * 1000)/ run_time;
                LOG_DEBUG("monitor", "%-10s %d.%-4d    %-4d   %-4d",
                          gloable_taskManager->task_tab[i]->task_tcb->name,
                          gloable_taskManager->task_tab[i]->task_tcb->cpu_usage_info.usage_percent / 10,
                          gloable_taskManager->task_tab[i]->task_tcb->cpu_usage_info.usage_percent % 10,
                          linear_pool_get()->size,
                          gloable_taskManager->task_tab[i]->task_tcb->stack_left);
                gloable_taskManager->task_tab[i]->task_tcb->cpu_usage_info.total_run_time = 0;
            }
        }
        LOG_DEBUG("monitor", "--------------------------------");
        LOG_DEBUG("monitor", "----------------monitor unlock----------------");
        gloable_mutex->fun->mutex_unlock(gloable_mutex);

    }
}



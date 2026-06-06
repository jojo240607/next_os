#include "log_task.h"
#include "../../common/linear_pool.h"
#include "../../driver/svc.h"
#include <stdio.h>
#include <stdlib.h>


task_init_override(log_task_task_init_impl);
task_thread_override(log_task_task_thread_impl);

// 析构函数声明
static void log_task_destroy(Log_task* self);

// TODO: 初始化数据成员
static const Log_taskFun log_task_fun = {
    .destroy = log_task_destroy,
};

/* 级别字符串 */
static const char *level_str[] = {
        [LOG_LEVEL_DEBUG] = "D",
        [LOG_LEVEL_INFO]  = "I",
        [LOG_LEVEL_WARN]  = "W",
        [LOG_LEVEL_ERROR] = "E",
        [LOG_LEVEL_FATAL] = "F"
};

// 构造函数实现
Log_task* log_task_create(const task_into_t *info) {
    Log_task* obj = (Log_task*)os_malloc(sizeof(Log_task));
    if (obj) {
        memset(obj, 0, sizeof(Log_task));
        log_task_init(obj, info);
    }
    return obj;
}

void log_task_init(Log_task* self, const task_into_t *info) {
    // 初始化基类部分
    task_init(&self->base, info);
    self->fun = &(log_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = log_task_task_init_impl;
	def_task_thread(self) = log_task_task_thread_impl;
    self->usart = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_USART1);
#ifndef LOG_USE_NOCOPY
    self->entry = os_malloc(sizeof(log_entry_t));
    memset(self->entry, 0, sizeof(log_entry_t));
#endif
}

void log_task_deinit(Log_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void log_task_destroy(Log_task* self) {
    if (self != NULL) {
        log_task_deinit(self);
        os_free(self);
    }
}

// task_init method
//add task ---> init task
task_init_override(log_task_task_init_impl) {
    // TODO: add task_init method
    LOG_DEBUG("log","log_task init");
    Log_task *log_task = (Log_task *)self;
    //params , void *parent
    if (!log_task) {
        return;
    }
    log_bound_task(gloable_log, self);
}
//char test[] = "ssssdddd";
// task_thread method
task_thread_override(log_task_task_thread_impl) {
    // TODO: add task_thread method
    Log_task *log_task = (Log_task *)self->parent;
    //params , void *arg
    while (1) {
        sem_take_user(GET_TASK(log_task)->task_tcb->semaphore);
        /* 批量处理，直到缓冲区空 */
#ifndef LOG_USE_NOCOPY
        while (log_get_data(gloable_log, log_task->entry)) {
#else
        log_task->entry = log_get_data_nocpy(gloable_log);
        while (log_task->entry) {
#endif
            /* 格式化并发送到串口 */
            int len = snprintf(log_task->line, sizeof(log_task->line),
                               "%08lu %d %s %s: %s",
                               log_task->entry->timestamp,
                               log_task->entry->tid,
                               level_str[log_task->entry->level],
                               log_task->entry->tag,
                               log_task->entry->text);
            if (len > 0) {

  //              log_task->usart->fun->write_user(log_task->usart, (const uint8_t *)test, 8);
                log_task->usart->fun->write_user(log_task->usart, (const uint8_t *)log_task->line, len);
            }
#ifdef LOG_USE_NOCOPY
            log_task->entry = log_get_data_nocpy(gloable_log);
#endif
        }
    }
}


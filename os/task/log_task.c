#include "log_task.h"
#include "../common/linear_pool.h"
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
Log_task* log_task_create() {
    Log_task* obj = (Log_task*)os_malloc(sizeof(Log_task));
    if (obj) {
        memset(obj, 0, sizeof(Log_task));
        log_task_init(obj);
    }
    return obj;
}

void log_task_init(Log_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(log_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = log_task_task_init_impl;
	def_task_thread(self) = log_task_task_thread_impl;
    self->log_buf = queue_create();
    self->usart = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_USART4);
}

void log_task_deinit(Log_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
    if (self->log_buf) {
        //String *str = GET_STRING(self->log_buf->fun->dequeue(self->log_buf));
        //while (str) {
        //    str->fun->destroy(str);
        //    str = GET_STRING(self->log_buf->fun->dequeue(self->log_buf));
        //}
        self->log_buf->fun->destroy(self->log_buf);
    }
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
    virtual_dev_init(log_task->usart, self->task_tcb->semaphore);
    log_setsem(self->task_tcb->semaphore);
}
// task_thread method
task_thread_override(log_task_task_thread_impl) {
    // TODO: add task_thread method
    Log_task *log_task = (Log_task *)self->parent;
    //params , void *arg
    Ring *log_buf = log_buffer();
    log_entry_t entry;
    while (1) {
        self->semaphore->fun->take(self->semaphore);
        /* 批量处理，直到缓冲区空 */
        while (log_buf->fun->pop(log_buf, &entry)) {
            /* 格式化并发送到串口 */
            int len = snprintf(log_task->line, sizeof(log_task->line),
                               "%08lu %d %s %s: %s",
                               entry.timestamp,
                               entry.tid,
                               level_str[entry.level],
                               entry.tag,
                               entry.text);
            if (len > 0) {
                GET_USART(log_task->usart)->fun->send_it(GET_USART(log_task->usart), (const uint8_t *)log_task->line, len);
            }
        }
    }
}


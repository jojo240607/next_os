#include <stdarg.h>
#include <string.h>
#include <stdio.h>      // vsnprintf，需确保库函数是可重入的或已保护
#include <stdbool.h>
#include "log.h"
#include "../common/sys_time.h"
#include "../driver/common/device.h"
#include "../driver/usart.h"
#include "../task/os/log_task.h"
#include "../common/linear_pool.h"



Log *gloable_log = NULL;

/* ------- 日志条目与环形缓冲区 ------- */
Log *log_create() {
    Log* obj = (Log*)os_malloc(sizeof(Log));
    if (obj) {
        memset(obj, 0, sizeof(Log));
        log_init(obj);
    }
    return obj;
}
/* 初始化日志系统 */
void log_init(Log *self) {
    // 初始化基类部分
    self->ring_buf = ring_create(LOG_RING_BUF_SIZE, sizeof(log_entry_t));
    self->log_task = NULL;
}


/* 核心输出函数，由宏调用 */
void log_output(Log *self, log_level_t level, const char * tag, const char *fmt, ...) {
    if (!LOG_FILTER(level)) {
        return;
    }
    if (self->ring_buf == NULL) {
        return;
    }

    log_entry_t entry;
    entry.timestamp = get_systime_us() ? get_systime_us()->time_us : 0;
    entry.level = (uint8_t)level;
    entry.tid = (global_thread_scheduler && global_thread_scheduler->current_thread) ? global_thread_scheduler->current_thread->tid : 0;
    entry.tag = tag;

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.text, LOG_MSG_MAX_LEN, fmt, args);
    va_end(args);

    if (self->ring_buf->fun->push(self->ring_buf, &entry)) {
        if (self->log_task) {
            self->log_task->fun->trigger(self->log_task, NULL);
        }
    } else {
        // 缓冲区满，记录丢弃次数（可选，这里省略）
        if (self->log_task) {
            self->log_task->fun->trigger(self->log_task, NULL);
        }
    }
}

void log_directly_error(Log *self, const char *fmt, ...) {
    if (self->log_task == NULL) {
        return;
    }
    Log_task *log_task = (Log_task *)self->log_task;

    log_entry_t entry;
    entry.timestamp = get_systime_us()->time_us;
    entry.level = (uint8_t)LOG_LEVEL_ERROR;
    entry.tid = 0;
    entry.tag = "os_error";

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(entry.text, LOG_MSG_MAX_LEN, fmt, args);
    va_end(args);
    GET_USART(log_task->usart)->fun->send(GET_USART(log_task->usart), (const uint8_t *)entry.text, len);
}


void log_bound_task(Log *self, Task *task) {
    if (!task) {
        return;
    }
    self->log_task = task;
    if (self->ring_buf->count > 0) {
        self->log_task ->fun->trigger(task, NULL);
    }
}

void *log_get_data_nocpy(Log *self) {
    return self->ring_buf->fun->pop_nocpy(self->ring_buf);
}

bool log_get_data(Log *self, void * data) {
    return self->ring_buf->fun->pop(self->ring_buf, data);
}
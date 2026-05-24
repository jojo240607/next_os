#include <stdarg.h>
#include <string.h>
#include <stdio.h>      // vsnprintf，需确保库函数是可重入的或已保护
#include <stdbool.h>
#include "log.h"
#include "../common/ring.h"
#include "../scheduler/thread_scheduler.h"
#include "../common/sys_time.h"

/* ------- 日志条目与环形缓冲区 ------- */

/*
typedef struct {
    log_entry_t buffer[LOG_RING_BUF_SIZE];
    volatile uint32_t write_idx;    // 生产者写入位置
    volatile uint32_t read_idx;     // 消费者读取位置
} log_ring_buf_t;
*/
static Ring *log_buf;
static Semaphore *log_sem;            // 信号量，用于唤醒日志任务
Ring * log_buffer() {
    return log_buf;
}
/* 唤醒日志任务（ISR安全） */
/*static inline void wakeup_log_task(void) {
    if (os_is_in_isr()) {
        os_sem_signal_from_isr(&log_sem);
    } else {
        os_sem_signal(&log_sem);
    }
}*/

/* 压入一条日志到环形缓冲区，返回是否成功 */
/*
static bool log_buf_push(log_entry_t *entry) {
    uint32_t next = (log_buf.write_idx + 1) & (LOG_RING_BUF_SIZE - 1);
    if (next == log_buf.read_idx) {
        return false;   // 缓冲区满，丢弃
    }

    ENTER_CRITICAL();
    memcpy(&log_buf.buffer[log_buf.write_idx], entry, sizeof(log_entry_t));
    log_buf.write_idx = next;
    EXIT_CRITICAL();

    return true;
}


static bool log_buf_pop(log_entry_t *out) {
    if (log_buf.read_idx == log_buf.write_idx) {
        return false;   // 空
    }
    memcpy(out, &log_buf.buffer[log_buf.read_idx], sizeof(log_entry_t));
    log_buf.read_idx = (log_buf.read_idx + 1) & (LOG_RING_BUF_SIZE - 1);
    return true;
}*/



/* 日志输出任务 */
/*
static void log_task(void *p) {
    (void)p;
    log_entry_t entry;

    while (1) {
        os_sem_wait(&log_sem, 0xFFFFFFFF);  // 永久等待

        // 批量处理，直到缓冲区空
        while (log_buf->fun->pop(log_buf, &entry)) {
            // 格式化并发送到串口
            char line[LOG_MSG_MAX_LEN + 32];  // 额外空间给前缀
            int len = snprintf(line, sizeof(line),
                               "[%08lu][%s][M%d] %s",
                               entry.timestamp,
                               level_str[entry.level],
                               entry.module_id,
                               entry.text);
            if (len > 0) {
                serial_send_blocking(line, len);
            }
        }
    }
}*/

/* 核心输出函数，由宏调用 */
void log_output(log_level_t level, const char * tag, const char *fmt, ...) {
    if (!LOG_FILTER(level)) {
        return;
    }
    if (log_buf == NULL) {
        return;
    }

    log_entry_t entry;
    entry.timestamp = get_systime_us();//getSystime()->systick;// global_thread_scheduler->current_thread->tid;//os_get_tick();
    entry.level = (uint8_t)level;
    entry.tid = global_thread_scheduler == NULL ? 0 : global_thread_scheduler->current_thread->tid;
    entry.tag = tag;

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.text, LOG_MSG_MAX_LEN, fmt, args);
    va_end(args);

    if (log_buf->fun->push(log_buf, &entry)) {
        if (log_sem) {
            log_sem->fun->give(log_sem);
        }
    } else {
        // 缓冲区满，记录丢弃次数（可选，这里省略）
        if (log_sem) {
            log_sem->fun->give(log_sem);
        }
    }
}

/* 初始化日志系统 */
void log_init(void) {
    log_buf = ring_create(LOG_RING_BUF_SIZE, sizeof(log_entry_t));
    /* 创建信号量，初值 0 */
    //os_sem_create_binary(&log_sem, 0);

    /* 创建日志任务，需要你提供一个静态栈空间 */
    //static uint32_t log_task_stack[LOG_TASK_STACK_SIZE];
    //os_task_create(log_task, "logTask", LOG_TASK_STACK_SIZE * sizeof(uint32_t),
    //               NULL, LOG_TASK_PRIORITY, log_task_stack);
}

void log_setsem(Semaphore *sem) {
    log_sem = sem;
    if (log_buf->count > 0) {
        log_sem->fun->give(log_sem);
    }
}
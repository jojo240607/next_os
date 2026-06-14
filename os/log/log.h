//
// Created by Administrator on 2026/4/26/026.
//

#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include "log_config.h"
#include "../common/ring.h"
#include "../scheduler/semaphore.h"
#include "../task/task.h"

#define LOG_GLOBAL_LEVEL 3
/* 日志级别 */
typedef enum : uint8_t {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

/* 模块ID，可扩展 */
typedef enum : uint8_t {
    MODULE_SYSTEM = 0,
    MODULE_UART,
    MODULE_TASK,
    MODULE_USER0,
    MODULE_USER1,
    // ...
} module_id_t;
typedef struct {
    uint32_t    timestamp;          // 系统tick
    uint8_t     level;
    uint16_t tid;
    const char*     tag;
    char        text[LOG_MSG_MAX_LEN];
} log_entry_t;

typedef struct {
    Ring *ring_buf;
    Task *log_task;            // 日志任务，串口发送或者保存
} Log;

extern Log *gloable_log;

/* ------- 日志打印宏 ------- */

/*
 * 通用日志宏，用法：LOG(LOG_LEVEL_INFO, MODULE_UART, "baud: %d\n", 115200)
 * 注意：fmt 后面会自动换行，无需加 \n
 */
#define LOG(level, tag, fmt, ...) \
    do { \
        log_output(gloable_log, level, tag, fmt "\r\n", ##__VA_ARGS__); \
    } while(0)

/* 便捷级别宏 */
#define LOG_DEBUG(tag, fmt, ...)   LOG(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(module, fmt, ...)    LOG(LOG_LEVEL_INFO,  module, fmt, ##__VA_ARGS__)
#define LOG_WARN(module, fmt, ...)    LOG(LOG_LEVEL_WARN,  module, fmt, ##__VA_ARGS__)
#define LOG_ERROR(module, fmt, ...)   LOG(LOG_LEVEL_ERROR, module, fmt, ##__VA_ARGS__)
#define LOG_FATAL(module, fmt, ...)   LOG(LOG_LEVEL_FATAL, module, fmt, ##__VA_ARGS__)

/* 如果定义了 LOG_GLOBAL_LEVEL，则编译时过滤 */
#ifdef LOG_GLOBAL_LEVEL
#define LOG_FILTER(level) ((level) >= LOG_GLOBAL_LEVEL)
#else
#define LOG_FILTER(level) (1)
#endif

Log * log_create();
/* 初始化日志系统（创建日志任务） */
void log_init(Log *self);

/* 实际调用函数 */
void log_output(Log *self, log_level_t level, const char *tag, const char *fmt, ...);
Ring * log_buffer();
void log_bound_task(Log *self, Task *task);
void log_directly_error(Log *self, const char *fmt, ...);
void *log_get_data_nocpy(Log *self);
bool log_get_data(Log *self, void * data);
#endif

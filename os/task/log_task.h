#ifndef LOG_TASK_H
#define LOG_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "Base_task.h"
#include "../driver/usart.h"
#include "String.h"
#include "../driver/device_manager.h"
#define GET_LOG_TASK_VTABLE(obj) GET_BASE_TASK_VTABLE(obj) //(*(Log_taskVTable **)obj)
#define GET_LOG_TASK(obj) ((Log_task *)obj)

// 派生类声明
typedef struct _Log_task Log_task;
typedef struct _Log_taskFun Log_taskFun;

// 类成员函数结构
struct _Log_taskFun {
    void (*destroy)(Log_task* self);
};

struct _Log_task {
    Base_task base;  // 基类作为第一个成员
    const Log_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
    Usart *usart;
    Queue *log_buf;//String
};

// 构造函数声明
Log_task* log_task_create();
void log_task_init(Log_task* self);

// 析构函数声明
void log_task_deinit(Log_task* self);

#endif // LOG_TASK_H
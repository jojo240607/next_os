#ifndef MONITOR_TASK_H
#define MONITOR_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "../task.h"
#include "idle_task.h"

#define GET_MONITOR_TASK_VTABLE(obj) GET_BASE_TASK_VTABLE(obj) //(*(Monitor_taskVTable **)obj)
#define GET_MONITOR_TASK(obj) ((Monitor_task *)obj)

// 派生类声明
typedef struct _Monitor_task Monitor_task;
typedef struct _Monitor_taskFun Monitor_taskFun;
// 类成员函数结构
struct _Monitor_taskFun {
    void (*destroy)(Monitor_task* self);

};
struct _Monitor_task {
    Task base;  // 基类作为第一个成员
    const Monitor_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
    uint32_t last_call;
};

// 构造函数声明
Monitor_task* monitor_task_create(const task_into_t *info);
void monitor_task_init(Monitor_task* self, const task_into_t *info);

// 析构函数声明
void monitor_task_deinit(Monitor_task* self);

#endif // MONITOR_TASK_H
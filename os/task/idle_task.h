#ifndef IDLE_TASK_H
#define IDLE_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "task.h"

#define GET_IDLE_TASK_VTABLE(obj) GET_BASE_TASK_VTABLE(obj) //(*(Idle_taskVTable **)obj)
#define GET_IDLE_TASK(obj) ((Idle_task *)obj)

// 派生类声明
typedef struct _Idle_task Idle_task;
typedef struct _Idle_taskFun Idle_taskFun;
// 类成员函数结构
struct _Idle_taskFun {
    void (*destroy)(Idle_task* self);
};
struct _Idle_task {
    Task base;  // 基类作为第一个成员
    const Idle_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Idle_task* idle_task_create();
void idle_task_init(Idle_task* self);

// 析构函数声明
void idle_task_deinit(Idle_task* self);

#endif // IDLE_TASK_H
#ifndef SYSTICK_TASK_H
#define SYSTICK_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "Base_task.h"
#include "../driver/systick.h"

#define GET_SYSTICK_TASK_VTABLE(obj) GET_BASE_TASK_VTABLE(obj) //(*(Systick_taskVTable **)obj)
#define GET_SYSTICK_TASK(obj) ((Systick_task *)obj)

// 派生类声明
typedef struct _Systick_task Systick_task;
typedef struct _Systick_taskFun Systick_taskFun;
// 类成员函数结构
struct _Systick_taskFun {
    void (*destroy)(Systick_task* self);
};
struct _Systick_task {
    Base_task base;  // 基类作为第一个成员
    const Systick_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
    Device *systick;
};

// 构造函数声明
Systick_task* systick_task_create();
void systick_task_init(Systick_task* self);

// 析构函数声明
void systick_task_deinit(Systick_task* self);

#endif // SYSTICK_TASK_H
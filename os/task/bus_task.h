#ifndef BUS_TASK_H
#define BUS_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "Task.h"

#define GET_BUS_TASK_VTABLE(obj) GET_TASK_VTABLE(obj) //(*(Bus_taskVTable **)obj)
#define GET_BUS_TASK(obj) ((Bus_task *)obj)

// 派生类声明
typedef struct _Bus_task Bus_task;
typedef struct _Bus_taskFun Bus_taskFun;
// 类成员函数结构
struct _Bus_taskFun {
    void (*destroy)(Bus_task* self);
};
struct _Bus_task {
    Task base;  // 基类作为第一个成员
    const Bus_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
    Device *bus;
};

// 构造函数声明
Bus_task* bus_task_create();
void bus_task_init(Bus_task* self);

// 析构函数声明
void bus_task_deinit(Bus_task* self);

#endif // BUS_TASK_H
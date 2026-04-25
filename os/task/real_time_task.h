#ifndef REAL_TIME_TASK_H
#define REAL_TIME_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "base_task.h"
#include "../scheduler/semaphore.h"

#define GET_REAL_TIME_TASK_VTABLE(obj) (*(Real_time_taskVTable **)obj)
#define top_half_task_override(func_name) static void func_name(Real_time_task* self)
#define def_top_half_task(obj) (GET_REAL_TIME_TASK_VTABLE(obj)->top_half_task)
#define virtual_top_half_task(obj, ...) def_top_half_task(obj)(obj, ##__VA_ARGS__)

#define button_half_task_override(func_name) static void func_name(Real_time_task* self, void *arg)
#define def_button_half_task(obj) (GET_REAL_TIME_TASK_VTABLE(obj)->button_half_task)
#define virtual_button_half_task(obj, ...) def_button_half_task(obj)(obj, ##__VA_ARGS__)

#define GET_REAL_TIME_TASK(obj) ((Real_time_task *)obj)

// 派生类声明
typedef struct _Real_time_task Real_time_task;
typedef struct _Real_time_taskFun Real_time_taskFun;
typedef struct _Real_time_taskVTable Real_time_taskVTable;
// 虚函数表结构
typedef struct _Real_time_taskVTable {
    Base_taskVTable vtbase;
    // TODO: 添加其他虚函数
	void (*top_half_task)(Real_time_task* self);
    Tcb_entry button_half_task;

};
// 类成员函数结构
struct _Real_time_taskFun {
    void (*destroy)(Real_time_task* self);
	void (*quick_task)(Real_time_task* self);

};
struct _Real_time_task {
    Base_task base;  // 基类作为第一个成员
    const Real_time_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Real_time_task* real_time_task_create(const char *name);
void real_time_task_init(Real_time_task* self, const char *name);

// 析构函数声明
void real_time_task_deinit(Real_time_task* self);

#endif // REAL_TIME_TASK_H
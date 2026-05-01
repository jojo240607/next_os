#ifndef TIME_TASK_H
#define TIME_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "../task/task.h"
#include "../driver/timer.h"

#define GET_TIME_TASK_VTABLE(obj) GET_TASK_VTABLE(obj) //(*(Time_taskVTable **)obj)
#define GET_TIME_TASK(obj) ((Time_task *)obj)

// 派生类声明
typedef struct _Time_task Time_task;
typedef struct _Time_taskFun Time_taskFun;
// 类成员函数结构
struct _Time_taskFun {
    void (*destroy)(Time_task* self);
};
struct _Time_task {
    Task base;  // 基类作为第一个成员
    const Time_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
    Device *timer;
};

// 构造函数声明
Time_task* time_task_create();
void time_task_init(Time_task* self);

// 析构函数声明
void time_task_deinit(Time_task* self);

#endif // TIME_TASK_H
#ifndef OS_CB_TASK_H
#define OS_CB_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "../task.h"
#include "../../driver/exti.h"

#define GET_OS_CB_TASK_VTABLE(obj) GET_TASK_VTABLE(obj) //(*(Os_cb_taskVTable **)obj)
#define GET_OS_CB_TASK(obj) ((Os_cb_task *)obj)

// 派生类声明
typedef struct _Os_cb_task Os_cb_task;
typedef struct _Os_cb_taskFun Os_cb_taskFun;

typedef struct _os_callback os_callback;

// 类成员函数结构
struct _Os_cb_taskFun {
    void (*destroy)(Os_cb_task* self);
};

typedef void (*callback_handler)(exti_event_t *event, void *arg);

struct _os_callback {
    Node base;
    event_source_t cb_source;
    void *arg;
    callback_handler cb_handler;
};
struct _Os_cb_task {
    Task base;  // 基类作为第一个成员
    const Os_cb_taskFun* fun;
    // TODO: 添加派生类特有的数据成员

    Device *exti;
    Queue *callback_list;// exit_callback 队列
};

// 构造函数声明
Os_cb_task* os_cb_task_create(const task_into_t *info);
void os_cb_task_init(Os_cb_task* self, const task_into_t *info);

// 析构函数声明
void os_cb_task_deinit(Os_cb_task* self);
void register_callback(os_callback *callback);

#endif // OS_CB_TASK_H
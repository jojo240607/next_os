#ifndef OS_CB_TASK_H
#define OS_CB_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "Task.h"
#include "../driver/exti.h"

#define GET_OS_CB_TASK_VTABLE(obj) GET_TASK_VTABLE(obj) //(*(Os_cb_taskVTable **)obj)
#define GET_OS_CB_TASK(obj) ((Os_cb_task *)obj)

// 派生类声明
typedef struct _Os_cb_task Os_cb_task;
typedef struct _Os_cb_taskFun Os_cb_taskFun;
typedef enum _os_cb_event os_cb_event;
typedef struct _os_callback os_callback;
typedef void (*callback_handler)(os_cb_event event, void *arg);

// 类成员函数结构
struct _Os_cb_taskFun {
    void (*destroy)(Os_cb_task* self);
};

enum _os_cb_event {
    OS_EVENT_EXTI0 = 0xe0,
    OS_EVENT_EXTI1,
    OS_EVENT_EXTI2,
    OS_EVENT_EXTI3,
    OS_EVENT_EXTI4,
    OS_EVENT_EXTI5,
    OS_EVENT_EXTI6,
    OS_EVENT_EXTI7,
    OS_EVENT_EXTI8,
    OS_EVENT_EXTI9,
    OS_EVENT_EXTI10,
    OS_EVENT_EXTI11,
    OS_EVENT_EXTI12,
    OS_EVENT_EXTI13,
    OS_EVENT_EXTI14,
    OS_EVENT_EXTI15,
};
struct _os_callback {
    Node base;
    os_cb_event cb_event;
    void *arg;
    callback_handler cb_handler;
};
struct _Os_cb_task {
    Task base;  // 基类作为第一个成员
    const Os_cb_taskFun* fun;
    // TODO: 添加派生类特有的数据成员
    os_cb_event cb_event;
    Exti *exti;
    Queue *callback_list;// exit_callback 队列
};

// 构造函数声明
Os_cb_task* os_cb_task_create();
void os_cb_task_init(Os_cb_task* self);

// 析构函数声明
void os_cb_task_deinit(Os_cb_task* self);
void register_callback(os_callback *callback);

#endif // OS_CB_TASK_H
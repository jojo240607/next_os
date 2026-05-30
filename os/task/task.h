#ifndef Task_H
#define Task_H
/*
    override void task_init(void *parent);
    override void task_thread(void *arg);
    override void task_start();
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../scheduler/tcb_t.h"
#include "../scheduler/thread_scheduler.h"
//#include "../driver/common/nvic.h"
//#include "../driver/device_manager.h"


#define GET_TASK_VTABLE(obj) (*(TaskVTable **)obj)

#define task_init_override(func_name) static void func_name(Task* self, void *parent)
#define def_task_init(obj) (GET_TASK_VTABLE(obj)->task_init)
#define virtual_task_init(obj, ...) def_task_init(obj)(obj, ##__VA_ARGS__)

#define task_thread_override(func_name) static void func_name(Tcb_t* self, void *arg)
#define def_task_thread(obj) (GET_TASK_VTABLE(obj)->task_thread)
#define virtual_task_thread(obj, ...) def_task_thread(obj)(obj, ##__VA_ARGS__)

#define task_start_override(func_name) static void func_name(Task* self)
#define def_task_start(obj) (GET_TASK_VTABLE(obj)->task_start)
#define virtual_task_start(obj, ...) def_task_start(obj)(obj, ##__VA_ARGS__)

#define GET_TASK(obj) ((Task *)obj)
// 类声明
typedef struct _Task Task;
typedef struct _TaskFun TaskFun;
typedef struct _TaskVTable TaskVTable;
typedef struct _task_into_t task_into_t;
typedef Task* (*Task_create)(const task_into_t *info);
// 虚函数表结构
struct _TaskVTable {
    // TODO : 添加其他虚函数
	void (*task_init)(Task* self, void *parent);    //task初始化
    Tcb_loop task_thread;                          //task线程loop函数
	void (*task_start)(Task* self);

};

// 类成员函数结构
struct _TaskFun {
    void (*destroy)(Task* self);
	void (*add_thread)(Task* self);
	void (*trigger)(Task* self, void *event);
	void * (*get_event)(Task* self);

};

struct _task_into_t{
    const char *name;
    thread_priority_t priority;
    mpu_region_size_t stack_size;
    Task_create create;
} ;

// 类结构
struct _Task {
    TaskVTable* vtable;
    const TaskFun* fun;
    // TODO: 添加数据成员
    volatile Tcb_t *task_tcb;
    const task_into_t *info;
};

// 构造函数声明
Task* task_create(const task_into_t *info);
void task_init(Task* self, const task_into_t *info);

// 析构函数声明
void task_deinit(Task* self);

#endif // Task_H
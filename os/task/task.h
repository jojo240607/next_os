#ifndef Task_H
#define Task_H
/*
    override void task_init(void *parent);
    override void task_thread(void *arg);
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../scheduler/tcb_t.h"
#include "../scheduler/thread_scheduler.h"
#include "../driver/intc.h"
#include "../driver/device_manager.h"


#define GET_TASK_VTABLE(obj) (*(TaskVTable **)obj)

#define task_init_override(func_name) static void func_name(Task* self, void *parent)
#define def_task_init(obj) (GET_TASK_VTABLE(obj)->task_init)
#define virtual_task_init(obj, ...) def_task_init(obj)(obj, ##__VA_ARGS__)

#define task_thread_override(func_name) static void func_name(Tcb_t* self, void *arg)
#define def_task_thread(obj) (GET_TASK_VTABLE(obj)->task_thread)
#define virtual_task_thread(obj, ...) def_task_thread(obj)(obj, ##__VA_ARGS__)

#define GET_TASK(obj) ((Task *)obj)
// 类声明
typedef struct _Task Task;
typedef struct _TaskFun TaskFun;
typedef struct _TaskVTable TaskVTable;

// 虚函数表结构
typedef struct _TaskVTable {
    // TODO : 添加其他虚函数

	void (*task_init)(Task* self, void *parent);
    Tcb_entry task_thread;
};

// 类成员函数结构
struct _TaskFun {
    void (*destroy)(Task* self);
	void (*add_task)(Task* self, const char *name, uint8_t priority, size_t stack_size);

};
// 类结构
struct _Task {
    TaskVTable* vtable;
    const TaskFun* fun;
    // TODO: 添加数据成员
    Tcb_t *task_tcb;
};

// 构造函数声明
Task* task_create();
void task_init(Task* self);

// 析构函数声明
void task_deinit(Task* self);

#endif // Task_H
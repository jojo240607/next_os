#ifndef BASE_TASK_H
#define BASE_TASK_H
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


#define GET_BASE_TASK_VTABLE(obj) (*(Base_taskVTable **)obj)

#define task_init_override(func_name) static void func_name(Base_task* self, void *parent)
#define def_task_init(obj) (GET_BASE_TASK_VTABLE(obj)->task_init)
#define virtual_task_init(obj, ...) def_task_init(obj)(obj, ##__VA_ARGS__)

#define task_thread_override(func_name) static void func_name(Tcb_t* self, void *arg)
#define def_task_thread(obj) (GET_BASE_TASK_VTABLE(obj)->task_thread)
#define virtual_task_thread(obj, ...) def_task_thread(obj)(obj, ##__VA_ARGS__)

#define GET_BASE_TASK(obj) ((Base_task *)obj)
// 类声明
typedef struct _Base_task Base_task;
typedef struct _Base_taskFun Base_taskFun;
typedef struct _Base_taskVTable Base_taskVTable;

// 虚函数表结构
typedef struct _Base_taskVTable {
    // TODO : 添加其他虚函数

	void (*task_init)(Base_task* self, void *parent);
    Tcb_entry task_thread;
};
// 类成员函数结构
struct _Base_taskFun {
    void (*destroy)(Base_task* self);
	void (*add_task)(Base_task* self, const char *name, uint8_t priority, size_t stack_size);

};
// 类结构
struct _Base_task {
    Base_taskVTable* vtable;
    const Base_taskFun* fun;
    // TODO: 添加数据成员
    Tcb_t *task_tcb;
};

// 构造函数声明
Base_task* base_task_create();
void base_task_init(Base_task* self);

// 析构函数声明
void base_task_deinit(Base_task* self);

#endif // BASE_TASK_H
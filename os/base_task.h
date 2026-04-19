#ifndef BASE_TASK_H
#define BASE_TASK_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tcb_t.h"
#include "thread_schedule.h"

#define GET_BASE_TASK_VTABLE(obj) (*(Base_taskVTable **)obj)

#define init_override(func_name) static void func_name(Base_task* self)
#define def_init(obj) (GET_BASE_TASK_VTABLE(obj)->init)
#define virtual_init(obj, ...) def_init(obj)(obj, ##__VA_ARGS__)
#define thread_override(func_name) static void func_name(Tcb_t* self, void *arg)
#define def_thread(obj) (GET_BASE_TASK_VTABLE(obj)->thread)
#define virtual_thread(obj, ...) def_thread(obj)(obj, ##__VA_ARGS__)

#define GET_BASE_TASK(obj) ((Base_task *)obj)
// 类声明
typedef struct _Base_task Base_task;
typedef struct _Base_taskFun Base_taskFun;
typedef struct _Base_taskVTable Base_taskVTable;

// 虚函数表结构
typedef struct _Base_taskVTable {
    // TODO : 添加其他虚函数

	void (*init)(Base_task* self);
    void (*thread)(Tcb_t* self, void *arg);

};
// 类成员函数结构
struct _Base_taskFun {
    void (*destroy)(Base_task* self);
};
// 类结构
struct _Base_task {
    Base_taskVTable* vtable;
    const Base_taskFun* fun;
    // TODO: 添加数据成员
    Tcb_t *task_tcb;
};

// 构造函数声明
Base_task* base_task_create(const char *name);
void base_task_init(Base_task* self, const char *name);

// 析构函数声明
void base_task_deinit(Base_task* self);

#endif // BASE_TASK_H
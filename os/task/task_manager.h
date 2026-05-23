#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "task.h"
#include "../common/util.h"

// 类声明
typedef struct _Task_manager Task_manager;
typedef struct _Task_managerFun Task_managerFun;
typedef struct _Task_list Task_list;
typedef Task* (*Task_create)();
// 类成员函数结构
struct _Task_managerFun {
    void (*destroy)(Task_manager* self);
	void (*boot_init)(Task_manager* self);

};

typedef enum : uint8_t  {
    TASK_SYSTICK = 0,
    TASK_IDLE,
    TASK_OS_CALLBACK,
    TASK_MONITOR,
    TASK_LOG,
    TASK_TEST1,
    TASK_TIME,
} task_id_t;
struct _Task_list {
    const task_id_t id;
    const char *name;
    //Task *task;
    uint8_t task_priority;
    mpu_region_size_t stack_size;
    Task_create task_create;
};
// 类结构
struct _Task_manager {
    const Task_managerFun* fun;
    // TODO: 添加数据成员
    const Task_list* task_list;
    size_t task_size;
    Task *task_tab[];
};

// 构造函数声明
Task_manager* task_manager_create();
void task_manager_init(Task_manager* self);

// 析构函数声明
void task_manager_deinit(Task_manager* self);
extern Task_manager * gloable_taskManager;
#endif // TASK_MANAGER_H
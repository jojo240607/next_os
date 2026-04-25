#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "base_task.h"
#include "../common/util.h"

// 类声明
typedef struct _Task_manager Task_manager;
typedef struct _Task_managerFun Task_managerFun;
typedef struct _Task_list Task_list;
typedef enum _Task_Tag Task_tag;
typedef Base_task* (*Task_create)();
// 类成员函数结构
struct _Task_managerFun {
    void (*destroy)(Task_manager* self);
	void (*boot_init)(Task_manager* self);

};

enum _Task_Tag {
    TASK_IDLE = 0,
    TASK_MONITOR,
    TASK_LOG,
    TASK_TEST1,
};
struct _Task_list {
    const Task_tag tag;
    const char *name;
    //Base_task *task;
    uint8_t priority;
    Task_create task_create;
};
// 类结构
struct _Task_manager {
    const Task_managerFun* fun;
    // TODO: 添加数据成员
    const Task_list* task_list;
    size_t task_size;
    Base_task *task_tab[];
};

// 构造函数声明
Task_manager* task_manager_create();
void task_manager_init(Task_manager* self);

// 析构函数声明
void task_manager_deinit(Task_manager* self);

#endif // TASK_MANAGER_H
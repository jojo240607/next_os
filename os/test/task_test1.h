#ifndef TASK_TEST1_H
#define TASK_TEST1_H
#include <stdint.h>
#include <stdbool.h>
#include "../task/base_task.h"

#define GET_TASK_TEST1_VTABLE(obj) GET_BASE_TASK_VTABLE(obj) //(*(Task_test1VTable **)obj)
#define GET_TASK_TEST1(obj) ((Task_test1 *)obj)

// 派生类声明
typedef struct _Task_test1 Task_test1;
typedef struct _Task_test1Fun Task_test1Fun;
// 类成员函数结构
struct _Task_test1Fun {
    void (*destroy)(Task_test1* self);
};
struct _Task_test1 {
    Base_task base;  // 基类作为第一个成员
    const Task_test1Fun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Task_test1* task_test1_create(const char *name, uint8_t priority);
void task_test1_init(Task_test1* self, const char *name, uint8_t priority);

// 析构函数声明
void task_test1_deinit(Task_test1* self);

#endif // TASK_TEST1_H
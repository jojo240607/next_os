#ifndef SYSTICK_H
#define SYSTICK_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "../common/sys_time.h"

#define GET_SYSTICK_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(SystickVTable **)obj)
#define GET_SYSTICK(obj) ((Systick *)obj)

// 派生类声明
typedef struct _Systick Systick;
typedef struct _SystickFun SystickFun;
typedef struct _systick_config systick_conf;
// 类成员函数结构
struct _SystickFun {
    void (*destroy)(Systick* self);
};

struct _systick_config {
    uint32_t systick_frequency;
    //irq_config irq_conf;
};
struct _Systick {
    Device base;  // 基类作为第一个成员
    const SystickFun* fun;
    // TODO: 添加派生类特有的数据成员
    systick_conf *conf;
};

// 构造函数声明
Systick* systick_create(systick_conf *conf);
void systick_init(Systick* self, systick_conf *conf);

// 析构函数声明
void systick_deinit(Systick* self);
bool systick_irq_handler_impl(void *arg);

#endif // SYSTICK_H
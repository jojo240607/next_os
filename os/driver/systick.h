#ifndef SYSTICK_H
#define SYSTICK_H
#include <stdint.h>
#include <stdbool.h>
#include "Device.h"

#define GET_SYSTICK_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(SystickVTable **)obj)
#define GET_SYSTICK(obj) ((Systick *)obj)

// 派生类声明
typedef struct _Systick Systick;
typedef struct _SystickFun SystickFun;
// 类成员函数结构
struct _SystickFun {
    void (*destroy)(Systick* self);
};
struct _Systick {
    Device base;  // 基类作为第一个成员
    const SystickFun* fun;
    // TODO: 添加派生类特有的数据成员
    volatile uint32_t timetick;
};

// 构造函数声明
Systick* systick_create();
void systick_init(Systick* self);

// 析构函数声明
void systick_deinit(Systick* self);

#endif // SYSTICK_H
#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include <stdbool.h>
#include "Device.h"
#include "main.h"

#define GET_TIMER_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(TimerVTable **)obj)
#define GET_TIMER(obj) ((Timer *)obj)

// 派生类声明
typedef struct _Timer Timer;
typedef struct _TimerFun TimerFun;
typedef struct _timer_config timer_config;


struct _timer_config {
    TIM_TypeDef * timer_type;
    uint32_t time_frenqurncy;
    irq_config irq_conf;
};
// 类成员函数结构
struct _TimerFun {
    void (*destroy)(Timer* self);
};
struct _Timer {
    Device base;  // 基类作为第一个成员
    const TimerFun* fun;
    // TODO: 添加派生类特有的数据成员
    timer_config *conf;
};

// 构造函数声明
Timer* timer_create(timer_config *conf);
void timer_init(Timer* self, timer_config *conf);

// 析构函数声明
void timer_deinit(Timer* self);
bool timer_irq_handler_impl(void *arg);

#endif // TIMER_H
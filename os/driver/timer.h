#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "main.h"

#define GET_TIMER_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(TimerVTable **)obj)
#define GET_TIMER(obj) ((Timer *)obj)

// 派生类声明
typedef struct _Timer Timer;
typedef struct _TimerFun TimerFun;
typedef struct _timer_config timer_config;

/* time 编号 */
typedef enum {
    TIME_1 = 0,
    TIME_2,
    TIME_3,
    TIME_4,
    TIME_5,
    TIME_6,
    TIME_7,
    TIME_8,
    TIME_9,
    TIME_10,
    TIME_11,
    TIME_12,
    TIME_13,
    TIME_14,
    TIME_MAX
} time_id_t;

struct _timer_config {
    time_id_t timer_id;
    uint32_t time_frenqurncy;
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
#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "hal/hal_timer.h"

#define GET_TIMER_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(TimerVTable **)obj)
#define GET_TIMER(obj) ((Timer *)obj)

// 派生类声明
typedef struct _Timer Timer;
typedef struct _TimerFun TimerFun;


/* ---------- 定时器总配置 ---------- */
typedef struct {
    tim_id_t            id;
    tim_timebase_t      timebase;
    tim_it_t            it_enable;              // TIM_IT_UPDATE | TIM_IT_CC1 ...
    // 通道配置
    uint8_t             num_oc_channels;        // 输出比较通道数量
    tim_oc_channel_t    oc_channels[4];         // 最多4通道
    uint8_t             num_ic_channels;        // 输入捕获通道数量
    tim_ic_channel_t    ic_channels[4];
} tim_config_t;


// 类成员函数结构
struct _TimerFun {
    void (*destroy)(Timer* self);
	void (*start)(Timer* self);
	void (*stop)(Timer* self);
	void (*set_pulse)(Timer* self, uint8_t channel, uint32_t pulse);
	uint32_t (*get_capture)(Timer* self, uint8_t channel);
	uint32_t (*get_counter)(Timer* self);

	void (*set_period)(Timer* self, uint32_t autoreload);

};
struct _Timer {
    Device base;  // 基类作为第一个成员
    const TimerFun* fun;
    // TODO: 添加派生类特有的数据成员
    const tim_config_t *conf;
};

// 构造函数声明
Timer* timer_create(const tim_config_t *conf, const dev_pripority_t *priority);
void timer_init(Timer* self, const tim_config_t *conf, const dev_pripority_t *priority);

// 析构函数声明
void timer_deinit(Timer* self);

#endif // TIMER_H
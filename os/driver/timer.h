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



/* ---------- 计数模式 ---------- */
typedef enum : uint8_t {
    TIM_COUNTER_UP      = 0,
    TIM_COUNTER_DOWN    = 1,
    TIM_COUNTER_CENTER  = 2   // 中央对齐
} tim_counter_mode_t;

/* ---------- 输出比较 / PWM 模式 ---------- */
typedef enum : uint8_t {
    TIM_OC_MODE_FROZEN       = 0,   // 无动作
    TIM_OC_MODE_ACTIVE       = 1,
    TIM_OC_MODE_INACTIVE     = 2,
    TIM_OC_MODE_TOGGLE       = 3,
    TIM_OC_MODE_PWM1         = 6,
    TIM_OC_MODE_PWM2         = 7
} tim_oc_mode_t;

/* ---------- 输入捕获边沿 ---------- */
typedef enum : uint8_t {
    TIM_IC_EDGE_RISING   = 0,
    TIM_IC_EDGE_FALLING  = 1,
    TIM_IC_EDGE_BOTH     = 2
} tim_ic_edge_t;

/* ---------- 中断使能选项 ---------- */
typedef enum : uint16_t {
    TIM_IT_NONE        = 0,
    TIM_IT_UPDATE      = (1 << 0),  // 更新中断 (上溢/下溢/中央对齐)
    TIM_IT_CC1         = (1 << 1),  // 捕获/比较通道1中断
    TIM_IT_CC2         = (1 << 2),  // 捕获/比较通道2中断
    TIM_IT_CC3         = (1 << 3),  // 捕获/比较通道3中断
    TIM_IT_CC4         = (1 << 4),  // 捕获/比较通道4中断
    TIM_IT_COM         = (1 << 5),  // 换向中断 (仅高级定时器)
    TIM_IT_TRIGGER     = (1 << 6),  // 触发中断 (从模式触发)
    TIM_IT_BREAK       = (1 << 7),  // 刹车中断 (仅高级定时器)
    // 下面两个实际上是 DMA 请求使能，但常与中断一起管理
    TIM_DMA_UPDATE     = (1 << 8),  // 更新DMA请求使能
    TIM_DMA_CC1        = (1 << 9),  // 捕获/比较通道1 DMA请求使能
    TIM_DMA_CC2        = (1 << 10),
    TIM_DMA_CC3        = (1 << 11),
    TIM_DMA_CC4        = (1 << 12),
    TIM_DMA_COM        = (1 << 13), // 换向DMA请求使能
    TIM_DMA_TRIGGER    = (1 << 14), // 触发DMA请求使能
} tim_it_t;

/* ---------- 事件通知 ---------- */
typedef enum {
    TIM_EVT_UPDATE = 0,
    TIM_EVT_CC1,
    TIM_EVT_CC2,
    TIM_EVT_CC3,
    TIM_EVT_CC4
} tim_event_t;

/* ---------- 时基配置描述符 ---------- */
typedef struct {
    tim_counter_mode_t  counter_mode;    // 计数方向
    uint32_t            prescaler;       // 预分频值 (0..65535)
    uint32_t            autoreload;      // 自动重装载值 (0..65535 或 0..0xFFFF)
    uint8_t             clock_division;  // 死区发生器时钟分频 (通常 0)
    uint8_t             repetition;      // 重复计数 (仅高级定时器有效)
} tim_timebase_t;

/* ---------- 输出比较通道配置 ---------- */
typedef struct {
    uint8_t         channel;        // 1~4
    tim_oc_mode_t   mode;           // PWM1, PWM2, TOGGLE 等
    uint32_t        pulse;          // 比较值 (占空比)
    bool            enable_preload; // 使能预装载
    // 输出极性、空闲状态等可继续扩展
} tim_oc_channel_t;

/* ---------- 输入捕获通道配置 ---------- */
typedef struct {
    uint8_t         channel;        // 1~4
    tim_ic_edge_t   edge;           // 捕获边沿
    uint8_t         prescaler;      // 输入预分频 (0..3)
    uint8_t         filter;         // 数字滤波 (0..15)
} tim_ic_channel_t;

/* ---------- 定时器总配置 ---------- */
typedef struct {
    tim_id_t            id;
    tim_timebase_t      timebase;
    tim_it_t            it_enable;              // TIM_IT_UPDATE | TIM_IT_CC1 ...
    //tim_callback_t      callback;
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

};
struct _Timer {
    Device base;  // 基类作为第一个成员
    const TimerFun* fun;
    // TODO: 添加派生类特有的数据成员
    const tim_config_t *conf;
};

// 构造函数声明
Timer* timer_create(const tim_config_t *conf);
void timer_init(Timer* self, const tim_config_t *conf);

// 析构函数声明
void timer_deinit(Timer* self);
bool timer_irq_handler_impl(void *arg);

#endif // TIMER_H
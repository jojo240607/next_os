//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_TIMER_H
#define STM32F4DISCOVERY_HAL_TIMER_H

#include <stdbool.h>
#include "stdint.h"
#include "../common/nvic.h"

/* 定时器基址（STM32F407） */
#define xTIM1_BASE   0x40010000UL
#define xTIM2_BASE   0x40000000UL
#define xTIM3_BASE   0x40000400UL
#define xTIM4_BASE   0x40000800UL
#define xTIM5_BASE   0x40000C00UL
#define xTIM6_BASE   0x40001000UL
#define xTIM7_BASE   0x40001400UL
#define xTIM8_BASE   0x40010400UL
#define xTIM9_BASE   0x40014000UL
#define xTIM10_BASE  0x40014400UL
#define xTIM11_BASE  0x40014800UL
#define xTIM12_BASE  0x40001800UL
#define xTIM13_BASE  0x40001C00UL
#define xTIM14_BASE  0x40002000UL



/* time 编号 */
/* ---------- 定时器实例 ---------- */
typedef enum :uint8_t {
    TIM_1  = 0,
    TIM_2,
    TIM_3,
    TIM_4,
    TIM_5,
    TIM_6,
    TIM_7,
    TIM_8,
    TIM_9,
    TIM_10,
    TIM_11,
    TIM_12,
    TIM_13,
    TIM_14,
    TIM_MAX
} tim_id_t;


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

void hal_tim_clock_enable(tim_id_t id);
void hal_tim_timebase_init(tim_id_t id, const tim_timebase_t *timebase);
void hal_tim_oc_channel_init(tim_id_t id, uint8_t num_oc_channels, const tim_oc_channel_t *oc_channels);
void hal_tim_ic_channel_init(tim_id_t id, uint8_t num_ic_channels, const tim_ic_channel_t *ic_channels);
void hal_tim_disable_it(tim_id_t id);
bool hal_tim_it_init(tim_id_t id, tim_it_t it_enable);
nvic_irq_num hal_tim_get_irqn(tim_id_t id);

void hal_tim_start(tim_id_t id);
void hal_tim_stop(tim_id_t id);
void hal_tim_set_pulse(tim_id_t id, uint8_t channel, uint32_t pulse);
uint32_t hal_tim_get_capture(tim_id_t id, uint8_t channel);
uint32_t hal_tim_get_counter(tim_id_t id);
void hal_tim_set_period(tim_id_t id, uint32_t autoreload);
uint32_t hal_tim_get_it_event(tim_id_t id);
void hal_tim_clear_it_event(tim_id_t id, uint8_t it_event);

#endif //STM32F4DISCOVERY_HAL_TIMER_H

//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_TIMER_H
#define STM32F4DISCOVERY_HAL_TIMER_H
#include "stdint.h"

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


/* ---------- TIM 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;          // 重复计数，高级定时器
    volatile uint32_t CCR[4];       // 通道比较/捕获寄存器
    volatile uint32_t BDTR;         // 断路和死区，高级定时器
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} xTIM_TypeDef;
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
extern xTIM_TypeDef* const TIMx[TIM_MAX];

void hal_tim_clock_enable(tim_id_t id);
#endif //STM32F4DISCOVERY_HAL_TIMER_H

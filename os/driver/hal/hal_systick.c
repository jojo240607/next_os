//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_systick.h"
#include "../common/rcc.h"

/* ---------- SysTick 寄存器 (Cortex-M4) ---------- */
typedef struct {
    volatile uint32_t CTRL;     // 控制与状态
    volatile uint32_t LOAD;     // 重装载值
    volatile uint32_t VAL;      // 当前值
    volatile const uint32_t CALIB; // 校准 (只读)
} xSysTick_TypeDef;

#define xSYSTICK       ((xSysTick_TypeDef *)xSYSTICK_BASE)

void hal_systick_init(uint32_t interval_us) {
    /* 计算重装载值: 每微秒时钟周期数 = frequency_hz / 1000000 */
    uint32_t ticks_per_us = hal_rcc_get_system_clock() / 1000000UL;
    uint32_t reload = interval_us * ticks_per_us;

    /* 限制为 24 位 */
    if (reload > SYSTICK_MAX_RELOAD) {
        reload = SYSTICK_MAX_RELOAD;
    }

    /* 关闭定时器以确保安全配置 */
    xSYSTICK->CTRL = 0;
    xSYSTICK->LOAD = reload;
    xSYSTICK->VAL  = 0;  // 清除当前值
}


void hal_systick_start() {
    uint32_t ctrl = 0;
    /* 使用处理器时钟 (HCLK) */
    ctrl |= SYSTICK_CTRL_CLKSOURCE;   // 1: 内核时钟
    /* 使能中断*/
    ctrl |= SYSTICK_CTRL_TICKINT;
    /* 使能计数器 */
    ctrl |= SYSTICK_CTRL_ENABLE;
    xSYSTICK->CTRL = ctrl;
}

void hal_systick_stop() {
    xSYSTICK->CTRL &= ~SYSTICK_CTRL_ENABLE;
}


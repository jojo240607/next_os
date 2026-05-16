//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_SYSTICK_H
#define STM32F4DISCOVERY_HAL_SYSTICK_H
#include "stdint.h"


/* ---------- SysTick 寄存器 (Cortex-M4) ---------- */
typedef struct {
    volatile uint32_t CTRL;     // 控制与状态
    volatile uint32_t LOAD;     // 重装载值
    volatile uint32_t VAL;      // 当前值
    volatile const uint32_t CALIB; // 校准 (只读)
} xSysTick_TypeDef;

#define xSYSTICK_BASE  0xE000E010UL
#define xSYSTICK       ((xSysTick_TypeDef *)xSYSTICK_BASE)

/* CTRL 位定义 */
#define SYSTICK_CTRL_ENABLE    (1UL << 0)
#define SYSTICK_CTRL_TICKINT   (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)
#define SYSTICK_CTRL_COUNTFLAG (1UL << 16)

/* 最大重装载值 (24位) */
#define SYSTICK_MAX_RELOAD     0x00FFFFFFUL


#endif //STM32F4DISCOVERY_HAL_SYSTICK_H

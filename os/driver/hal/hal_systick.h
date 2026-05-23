//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_SYSTICK_H
#define STM32F4DISCOVERY_HAL_SYSTICK_H
#include "stdint.h"




#define xSYSTICK_BASE  0xE000E010UL


/* CTRL 位定义 */
#define SYSTICK_CTRL_ENABLE    (1UL << 0)
#define SYSTICK_CTRL_TICKINT   (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)
#define SYSTICK_CTRL_COUNTFLAG (1UL << 16)

/* 最大重装载值 (24位) */
#define SYSTICK_MAX_RELOAD     0x00FFFFFFUL

void hal_systick_init(uint32_t interval_us);
void hal_systick_start();
void hal_systick_stop();

#endif //STM32F4DISCOVERY_HAL_SYSTICK_H

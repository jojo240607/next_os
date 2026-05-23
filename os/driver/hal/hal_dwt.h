//
// Created by zhiwei.gong on 2026/5/21.
//

#ifndef STM32F4DISCOVERY_HAL_DWT_H
#define STM32F4DISCOVERY_HAL_DWT_H
#include <stdint.h>
#include <stdbool.h>

#define ENABLE_CYCLE_COUNTER true

/* ========== API ========== */
int  hal_dwt_init();
void hal_dwt_deinit(void);

/* 获取自上次初始化/复位以来的 CPU 周期数 (32 位) */
uint32_t hal_dwt_get_ticks(void);

/* 微秒/毫秒级阻塞延时 */
void hal_dwt_delay_us(uint32_t us);
void hal_dwt_delay_ms(uint32_t ms);

/* 获取两次调用之间的时间差 (微秒)，适合测量代码段执行时间 */
uint32_t hal_dwt_elapsed_us(uint32_t start_ticks);

/* 获取系统时间戳 (微秒)，基于周期计数器的累计值 (考虑 32 位溢出) */
uint64_t hal_dwt_get_timestamp_us(void);

#endif //STM32F4DISCOVERY_HAL_DWT_H

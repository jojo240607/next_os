//
// Created by zhiwei.gong on 2026/5/19.
//

#ifndef STM32F4DISCOVERY_HAL_FLASH_H
#define STM32F4DISCOVERY_HAL_FLASH_H
#include <stdint.h>
#include <stdbool.h>

/* Flash 等待周期 (等待状态数) */
typedef enum {
    FLASH_LATENCY_0WS = 0,   // 0 等待周期，≤ 30 MHz
    FLASH_LATENCY_1WS = 1,
    FLASH_LATENCY_2WS = 2,
    FLASH_LATENCY_3WS = 3,
    FLASH_LATENCY_4WS = 4,
    FLASH_LATENCY_5WS = 5,   // 5 等待周期，≤ 168 MHz (推荐)
    FLASH_LATENCY_6WS = 6,
    FLASH_LATENCY_7WS = 7
} flash_latency_t;

/* Flash 配置描述符 */
typedef struct {
    flash_latency_t latency;     // 等待周期
    bool            enable_prefetch;   // 使能预取指
    bool            enable_icache;     // 使能指令缓存
    bool            enable_dcache;     // 使能数据缓存
} flash_config_t;

/* ========== API ========== */
int  hal_flash_init(const flash_config_t *cfg);
void hal_flash_deinit(void);

/* 运行时修改等待周期 (需谨慎，通常不建议在运行中更改) */
int  hal_flash_set_latency(flash_latency_t latency);

/* 启用/禁用预取指和缓存 */
void hal_flash_enable_prefetch(bool enable);
void hal_flash_enable_icache(bool enable);
void hal_flash_enable_dcache(bool enable);

#endif //STM32F4DISCOVERY_HAL_FLASH_H

//
// Created by zhiwei.gong on 2026/5/18.
//

#ifndef STM32F4DISCOVERY_HAL_RNG_H
#define STM32F4DISCOVERY_HAL_RNG_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/rcc.h"

/* RNG 配置描述符（预留，目前没有可配置项） */
typedef struct {
    uint8_t reserved;   // 可扩展中断使能等
} rng_config_t;

/* API */
int  rng_init(const rng_config_t *cfg);
void rng_deinit(void);
uint32_t rng_get_random(void);
bool rng_is_ready(void);
bool rng_is_error(void);

#endif //STM32F4DISCOVERY_HAL_RNG_H

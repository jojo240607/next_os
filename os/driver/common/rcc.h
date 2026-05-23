#ifndef RCC_H
#define RCC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../hal/hal_rcc.h"





/* ---------- 外设时钟使能/禁用接口 (按总线+位号) ---------- */
void rcc_periph_clock_enable(rcc_bus_t bus, rcc_enter_t periph_bit);
void rcc_periph_clock_disable(rcc_bus_t bus, rcc_enter_t periph_bit);

/* ---------- 外设复位控制 ---------- */
void rcc_periph_reset_assert(rcc_bus_t bus, rcc_enter_t periph_bit);
void rcc_periph_reset_deassert(rcc_bus_t bus, rcc_enter_t periph_bit);

/* ---------- 系统复位 ---------- */
void rcc_system_reset(void);

/* ---------- 全局初始化 ---------- */
bool rcc_sysclk_init(const rcc_sysclk_config_t *cfg);


#endif /* RCC_H */
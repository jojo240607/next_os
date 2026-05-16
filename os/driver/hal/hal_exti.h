//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_EXTI_H
#define STM32F4DISCOVERY_HAL_EXTI_H
#include "stdint.h"
#include "hal_gpio.h"

/* EXTI 基地址 */
#define xEXTI_BASE  0x40013C00UL

/* ---------- EXTI 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t IMR;      // 中断屏蔽寄存器
    volatile uint32_t EMR;      // 事件屏蔽寄存器
    volatile uint32_t RTSR;     // 上升沿触发选择
    volatile uint32_t FTSR;     // 下降沿触发选择
    volatile uint32_t SWIER;    // 软件中断事件
    volatile uint32_t PR;       // 挂起寄存器
} xEXTI_TypeDef;

#define xEXTI       ((xEXTI_TypeDef *)xEXTI_BASE)

void hal_syscfg_clock_enable(void);
void hal_syscfg_exti_line_config(gpio_port_t port, uint8_t pin);
void hal_exti_set_trigger(uint8_t pin, exti_mode irq_mode);
void hal_exti_enable_irq(uint8_t pin);

#endif //STM32F4DISCOVERY_HAL_EXTI_H

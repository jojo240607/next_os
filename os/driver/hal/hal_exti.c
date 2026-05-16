//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_exti.h"
#include "hal_gpio.h"
#include "../common/rcc.h"


/* 使能 SYSCFG 时钟（通常在初始化时做一次） */
void hal_syscfg_clock_enable(void) {
    rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_SYSCFGEN);
}
/* 将 GPIO 端口映射到 EXTI 线 */
void hal_syscfg_exti_line_config(gpio_port_t port, uint8_t pin) {
    uint32_t shift = (pin % 4) * 4;
    uint32_t reg_idx = pin / 4;
    uint32_t temp = xSYSCFG->EXTICR[reg_idx];
    temp &= ~(0xFUL << shift);
    temp |= (uint32_t)port << shift;
    xSYSCFG->EXTICR[reg_idx] = temp;
}

/* 配置 EXTI 触发类型 */
void hal_exti_set_trigger(uint8_t pin, exti_mode irq_mode) {
    uint32_t bit = 1U << pin;
    switch (irq_mode) {
        case PIN_IRQ_MODE_RISING:
            xEXTI->RTSR |= bit;
            xEXTI->FTSR &= ~bit;
            break;
        case PIN_IRQ_MODE_FALLING:
            xEXTI->FTSR |= bit;
            xEXTI->RTSR &= ~bit;
            break;
        case PIN_IRQ_MODE_BOTH:
            xEXTI->RTSR |= bit;
            xEXTI->FTSR |= bit;
            break;
        default:
            break;
    }
}

/* 使能 EXTI 中断（NVIC） */
void hal_exti_enable_irq(uint8_t pin) {
    xEXTI->IMR |= (1U << pin);
}
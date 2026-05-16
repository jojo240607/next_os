//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_usart.h"
#include "../common/rcc.h"


xUSART_TypeDef* const USARTx[] = {
        (xUSART_TypeDef*)xUSART1_BASE,
        (xUSART_TypeDef*)xUSART2_BASE,
        (xUSART_TypeDef*)xUSART3_BASE,
        (xUSART_TypeDef*)xUSART4_BASE,
        (xUSART_TypeDef*)xUSART5_BASE,
        (xUSART_TypeDef*)xUSART6_BASE,
};

/* ---------- 时钟与频率配置 ---------- */
void hal_uart_clock_enable(uart_id_t id)
{
    switch (id) {
        case UART_1:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_USART1EN);
            break;
        case UART_2:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_USART2EN);
            break;
        case UART_3:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_USART3EN);
            break;
        case UART_4:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_UART4EN);
            break;
        case UART_5:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_UART5EN);
            break;
        case UART_6:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_USART6EN);
            break;
        default: break;
    }
    __asm volatile ("dsb" ::: "memory");
}
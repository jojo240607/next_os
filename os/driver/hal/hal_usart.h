//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_USART_H
#define STM32F4DISCOVERY_HAL_USART_H
#include "stdint.h"

#define xUSART1_BASE  0x40011000UL
#define xUSART2_BASE  0x40004400UL
#define xUSART3_BASE  0x40004800UL
#define xUSART4_BASE  0x40004C00UL
#define xUSART5_BASE  0x40005000UL
#define xUSART6_BASE  0x40011400UL





/* ---------- USART 寄存器 (STM32F4) ---------- */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} xUSART_TypeDef;
/* UART 编号 */
typedef enum : uint8_t {
    UART_1 = 0,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_6,
    UART_MAX
} uart_id_t;

extern xUSART_TypeDef* const USARTx[];
void hal_uart_clock_enable(uart_id_t id);
#endif //STM32F4DISCOVERY_HAL_USART_H

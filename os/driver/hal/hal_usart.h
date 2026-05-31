//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_USART_H
#define STM32F4DISCOVERY_HAL_USART_H

#include <stdbool.h>
#include <stddef.h>
#include "stdint.h"
#include "hal_dma.h"
#include "../common/dma.h"

#define xUSART1_BASE  0x40011000UL
#define xUSART2_BASE  0x40004400UL
#define xUSART3_BASE  0x40004800UL
#define xUSART4_BASE  0x40004C00UL
#define xUSART5_BASE  0x40005000UL
#define xUSART6_BASE  0x40011400UL


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

/* 字长 */
typedef enum : uint8_t {
    UART_WORDLEN_8  = 0x00,
    UART_WORDLEN_9  = 0x01,
} uart_word_len_t;


/* 停止位 */
typedef enum : uint8_t {
    UART_STOP_1     = 0x00,
    UART_STOP_0_5   = 0x01,
    UART_STOP_2     = 0x02,
    UART_STOP_1_5   = 0x03,
} uart_stop_t;
/* 校验 */
typedef enum : uint8_t {
    UART_PARITY_NONE  = 0x00,
    UART_PARITY_EVEN  = 0x02,
    UART_PARITY_ODD   = 0x03,
} uart_parity_t;

typedef enum : uint16_t {
    /* 中断使能选项 */
    xUART_IT_NONE = 0,

    /* 状态 / 中断使能位（位号匹配 SR 和 CR1） */
    xUART_IT_TXE   = (1 << 7),   // 发送数据寄存器空   (SR.7 / CR1.7)
    xUART_IT_RXNE  = (1 << 5),   // 接收数据寄存器非空 (SR.5 / CR1.5)
    xUART_IT_TC    = (1 << 6),   // 发送完成           (SR.6 / CR1.6)

    /* 以下是 SR 中的附加标志，一般只用于状态检查，不用于使能 */
    xUART_FLAG_PE   = (1 << 0),  // 奇偶校验错误
    xUART_FLAG_FE   = (1 << 1),  // 帧错误
    xUART_FLAG_NE   = (1 << 2),  // 噪声错误
    xUART_FLAG_ORE  = (1 << 3),  // 溢出错误
    xUART_FLAG_IDLE = (1 << 4),  // 空闲线路检测
    xUART_FLAG_RXNE = xUART_IT_RXNE,  // 别名，方便统一
    xUART_FLAG_TXE  = xUART_IT_TXE,
    xUART_FLAG_TC   = xUART_IT_TC,
} uart_it_t;


void hal_uart_clock_enable(uart_id_t id);
void hal_uart_set_baudrate(uart_id_t id, uint32_t baudrate);
void hal_uart_set_format(uart_id_t id, uart_word_len_t word_len, uart_stop_t stop_bits, uart_parity_t parity);
void hal_uart_enable(uart_id_t id);
bool hal_uart_it_init(uart_id_t id, uart_it_t it_enable);
void hal_uart_dma_init(uart_id_t id, bool txdma, bool rxdma);
void hal_uart_send(uart_id_t id, uint8_t *buf, size_t count);
void hal_uart_recv(uart_id_t id, uint8_t *buf, size_t count);
uint32_t hal_uart_get_it_event(uart_id_t id);
void hal_uart_set_it_event(uart_id_t id, uart_it_t it_event);
void hal_uart_clear_it_event(uart_id_t id, uart_it_t it_event);
volatile uint8_t *hal_uart_data_addr(uart_id_t id);
int uart_send_dma(uart_id_t id, const dma_stream_config_t *dma_cfg, const uint8_t *data, uint16_t len);

#endif //STM32F4DISCOVERY_HAL_USART_H

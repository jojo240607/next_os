//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_usart.h"
#include "../common/rcc.h"

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

static xUSART_TypeDef* const USARTx[] = {
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

void hal_uart_set_baudrate(uart_id_t id, uint32_t baudrate) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    // 3. 波特率 (假设 APB2=84MHz for USART1, APB1=42MHz for USART2/3)
    uint32_t pclk = (id == UART_1) ? hal_rcc_get_apb2_clock() : hal_rcc_get_apb1_clock();
    uart_ctrl->BRR = pclk / baudrate;
}

void hal_uart_set_format(uart_id_t id, uart_word_len_t word_len, uart_stop_t stop_bits, uart_parity_t parity) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uint32_t cr1 = (1 << 3) | (1 << 2);  // TE, RE
    cr1 |= (word_len & 0x01) << 12; // M
    cr1 |= (parity & 0x03) << 9;   // PS, PCE
    uart_ctrl->CR1 = cr1;
    uart_ctrl->CR2 = (stop_bits & 0x03) << 12;
}

void hal_uart_enable(uart_id_t id) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    // 5. 使能模块
    uart_ctrl->CR1 |= (1 << 13);  // UE
}

bool hal_uart_it_init(uart_id_t id, uart_it_t it_enable) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];

    // 6. 中断配置
    if (it_enable) {
        uint32_t cr1 = uart_ctrl->CR1;
        if (it_enable & xUART_IT_TXE) {
            cr1 |= xUART_IT_TXE;   // TXEIE
        }
        if (it_enable & xUART_IT_RXNE) {
            cr1 |= xUART_IT_RXNE;   // RXNEIE
        }
        if (it_enable & xUART_IT_TC) {
            cr1 |= xUART_IT_TC;   // TCIE
        }
        uart_ctrl->CR1 = cr1;
        return true;
    }
    return false;
}

void hal_uart_dma_init(uart_id_t id, bool txdma, bool rxdma) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    if (txdma) {
        // 使能 USART 的 DMA 发送位 CR3 bit7 (DMAT)
        uart_ctrl->CR3 |= (1 << 7);
    }
    if (rxdma) {
        uart_ctrl->CR3 |= (1 << 6); // DMAR
    }
}

void hal_uart_send(uart_id_t id, uint8_t *buf, size_t count) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    while (count--) {
        while (!(uart_ctrl->SR & (1 << 7)));
        uart_ctrl->DR = *(char *) buf++;
    }
}

void hal_uart_recv(uart_id_t id, uint8_t *buf, size_t count) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    while(!(uart_ctrl->SR & (1 << 5)));
    *(char *)buf = (char)uart_ctrl->DR;
}

uint32_t hal_uart_get_it_event(uart_id_t id) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uint32_t sr = uart_ctrl->SR;
    return sr;
}

void hal_uart_set_it_event(uart_id_t id, uart_it_t it_event) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uart_ctrl->CR1 |= it_event;   // 关闭 RXNE 中断
}

void hal_uart_clear_it_event(uart_id_t id, uart_it_t it_event) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uart_ctrl->CR1 &= ~(it_event);   // 关闭 RXNE 中断
}

volatile uint8_t *hal_uart_data_addr(uart_id_t id) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    return (volatile uint8_t *)&uart_ctrl->DR;
}


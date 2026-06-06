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
/*
 * #define USART_CR3_DMAT_Pos            (7U)
 * #define USART_CR3_DMAR_Pos            (6U)
 *
 * */
void hal_uart_dma_init(uart_id_t id, bool txdma, bool rxdma) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    if (txdma) {
        // 使能 USART 的 DMA 发送位 CR3 bit7 (DMAT)
        uart_ctrl->CR3 |= (1 << USART_CR3_DMAT_Pos);
    }
    if (rxdma) {
        uart_ctrl->CR3 |= (1 << USART_CR3_DMAR_Pos); // DMAR
    }
}

void hal_uart_send(uart_id_t id, uint8_t *buf, size_t count) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    while (count--) {
        while (!(uart_ctrl->SR & xUART_FLAG_TXE));
        uart_ctrl->DR = *(char *) buf++;
    }
}

size_t hal_uart_recv(uart_id_t id, uint8_t *buf, size_t count) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    while(!(uart_ctrl->SR & xUART_FLAG_RXNE));
    *(char *)buf = (char)uart_ctrl->DR;
    return 1;
}

uint32_t hal_uart_get_it_event(uart_id_t id) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uint32_t sr = uart_ctrl->SR;
    return sr;
}

void hal_uart_it_enable(uart_id_t id, uart_it_t it_event) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uart_ctrl->CR1 |= it_event;   // 关闭 RXNE 中断
}

void hal_uart_it_disable(uart_id_t id, uart_it_t it_event) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uart_ctrl->CR1 &= ~(it_event);   // 关闭 RXNE 中断
}

void hal_uart_it_clear(uart_id_t id, uart_it_t it_event) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uart_ctrl->SR &= ~(it_event);
}

volatile uint8_t *hal_uart_data_addr(uart_id_t id) {
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    return (volatile uint8_t *)&uart_ctrl->DR;
}

/* DMA 发送一个数据块
 * M2P 方向：dma_start_transfer(src, dst, len)
 *   → SxPAR=dst(外设=DR), SxM0AR=src(存储器=data)
 */
int uart_send_dma(uart_id_t id, const dma_stream_config_t *dma_cfg, const uint8_t *data, uint16_t len)
{
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    uart_ctrl->SR &= ~xUART_FLAG_TC;
    dma_start_transfer(dma_cfg, (uint32_t)data, (uint32_t)(&uart_ctrl->DR), len);
    return 0;
}

/* DMA 接收（启动循环 DMA 到环形缓冲区）
 * P2M 方向：dma_start_transfer(src, dst, len)
 *   → SxPAR=src(外设=DR), SxM0AR=dst(存储器=buf)
 */
int uart_recv_dma(uart_id_t id, const dma_stream_config_t *dma_cfg, uint8_t *buf, uint16_t buf_size)
{
    xUSART_TypeDef *uart_ctrl = USARTx[id];
    dma_start_transfer(dma_cfg, (uint32_t)(&uart_ctrl->DR), (uint32_t)buf, buf_size);
    return 0;
}

/* 获取 DMA 流当前的 NDTR 值（剩余传输数） */
uint16_t uart_dma_get_rx_ndtr(const dma_stream_config_t *dma_cfg)
{
    uint32_t stream_base = (DMA_REQ_GET_CTRL(dma_cfg->dma_request) == DMA_1) ?
            DMA1_STREAM_BASE(DMA_REQ_GET_STREAM(dma_cfg->dma_request)) :
            DMA2_STREAM_BASE(DMA_REQ_GET_STREAM(dma_cfg->dma_request));
    xDMA_Stream_TypeDef *dma = (xDMA_Stream_TypeDef *)stream_base;
    return (uint16_t)dma->SxNDTR;
}

/*
 * 安全清除 USART IDLE 标志（DMA 循环接收模式下使用）
 *
 * 问题背景：
 *   STM32F4 清除 IDLE 标志必须执行 "读 SR → 读 DR" 序列。
 *   但在 DMA 循环接收模式下，DMA 正持续从 USART_DR 搬运数据到内存。
 *   此时 CPU 直接读 DR 会与 DMA 争抢外设数据寄存器，导致 DMA 流控被打乱。
 *
 * 解决方案（依据 STM32F4 参考手册建议）：
 *   1. 暂停 DMA 流 (SxCR.EN=0)，DMA 停止访问 DR
 *   2. CPU 独占 DR，安全执行 SR→DR 读序列清除 IDLE
 *   3. 恢复 DMA 流 (SxCR.EN=1)，NDTR 保存不变，DMA 从断点继续
 *
 * 注意：不要通过操作 DMAR 来避免冲突——
 *   手册明确警告 "not to clear the DMAR bit without first
 *   clearing the DMA stream enable bit"。
 */
void hal_uart_clear_idle_flag(uart_id_t id, const dma_stream_config_t *dma_cfg)
{
    xUSART_TypeDef *uart = USARTx[id];

//   /* 获取 DMA 流寄存器指针 */
//   uint32_t stream_base = (DMA_REQ_GET_CTRL(dma_cfg->dma_request) == DMA_1) ?
//           DMA1_STREAM_BASE(DMA_REQ_GET_STREAM(dma_cfg->dma_request)) :
//           DMA2_STREAM_BASE(DMA_REQ_GET_STREAM(dma_cfg->dma_request));
//   xDMA_Stream_TypeDef *dma = (xDMA_Stream_TypeDef *)stream_base;

//   /*
//    * ① 暂停 DMA 流 —— 手册要求先关流再处理 DR
//    *    只清 EN(bit0)，保留所有配置（方向、循环、通道等）
//    *    NDTR 不会重载，保持当前值
//    */
//   uint32_t sxcr_saved = dma->SxCR;
//   dma->SxCR = sxcr_saved & ~1U;
//   __asm volatile ("dsb" ::: "memory");

    /*
     * ② CPU 独占 DR，安全清除 IDLE
     *    此时 DMA 流已停，不会与 CPU 争抢 DR
     */
    volatile uint32_t sr_val = uart->SR;
    volatile uint32_t dr_val = uart->DR;
    (void)sr_val;
    (void)dr_val;

//   /*
//    * ③ 恢复 DMA 流
//    *    SxCR 全部恢复（含 EN=1），NDTR 不变 → DMA 无缝续传
//    */
//   dma->SxCR = sxcr_saved;
//   __asm volatile ("dsb" ::: "memory");
}

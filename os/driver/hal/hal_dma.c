/**
 * hal_dma.c — DMA HAL 层（纯寄存器操作）
 * 所有 xDMA_Stream / xDMA_Base 寄存器访问统一在此文件中。
 * API 使用原始参数 (ctrl, stream) 以避免循环依赖。
 */
#include "hal_dma.h"
#include "../common/rcc.h"

/* ── 时钟使能 ── */
void hal_dma_clock_enable(dma_controller_t ctrl)
{
    if (ctrl == DMA_1) {
        rcc_periph_clock_enable(RCC_BUS_AHB1, xRCC_AHB1ENR_DMA1EN);
    } else {
        rcc_periph_clock_enable(RCC_BUS_AHB1, xRCC_AHB1ENR_DMA2EN);
    }
    __asm volatile ("dsb" ::: "memory");
}

void dma_clock_enable(dma_controller_t ctrl) {
    hal_dma_clock_enable(ctrl);
}

/* ── 获取寄存器指针 ── */
xDMA_Stream_TypeDef* hal_dma_get_stream(dma_controller_t ctrl, uint8_t stream)
{
    uint32_t base = (ctrl == DMA_1) ?
            DMA1_STREAM_BASE(stream) : DMA2_STREAM_BASE(stream);
    return (xDMA_Stream_TypeDef*)base;
}

xDMA_Base_TypeDef* hal_dma_get_base(dma_controller_t ctrl)
{
    return (ctrl == DMA_1) ?
            (xDMA_Base_TypeDef*)xDMA1_BASE : (xDMA_Base_TypeDef*)xDMA2_BASE;
}

/* ── Stream 控制 ── */
void hal_dma_stream_disable(dma_controller_t ctrl, uint8_t stream)
{
    hal_dma_get_stream(ctrl, stream)->SxCR &= ~(1u << 0);
}

void hal_dma_stream_enable(dma_controller_t ctrl, uint8_t stream)
{
    hal_dma_get_stream(ctrl, stream)->SxCR |= 1;
}

void hal_dma_stream_write_cr(dma_controller_t ctrl, uint8_t stream, uint32_t cr)
{
    hal_dma_get_stream(ctrl, stream)->SxCR = cr;
}

void hal_dma_stream_set_dir_addr(dma_controller_t ctrl, uint8_t stream,
                                  uint8_t dir, uint32_t src, uint32_t dst)
{
    xDMA_Stream_TypeDef *dma = hal_dma_get_stream(ctrl, stream);
    if (dir == 0) {         /* P2M: 外设→存储器 */
        dma->SxPAR  = src;
        dma->SxM0AR = dst;
    } else if (dir == 1) {  /* M2P: 存储器→外设 */
        dma->SxPAR  = dst;
        dma->SxM0AR = src;
    } else {                /* M2M */
        dma->SxPAR  = dst;
        dma->SxM0AR = src;
    }
}

void hal_dma_stream_set_ndtr(dma_controller_t ctrl, uint8_t stream, uint16_t count)
{
    hal_dma_get_stream(ctrl, stream)->SxNDTR = count;
}

uint16_t hal_dma_stream_get_ndtr(dma_controller_t ctrl, uint8_t stream)
{
    return (uint16_t)hal_dma_get_stream(ctrl, stream)->SxNDTR;
}

bool hal_dma_is_busy(dma_controller_t ctrl, uint8_t stream)
{
    xDMA_Stream_TypeDef *dma = hal_dma_get_stream(ctrl, stream);
    return (dma->SxNDTR != 0) && (dma->SxCR & 1);
}

/* ── 中断标志 ── */
void hal_dma_clear_flags(dma_controller_t ctrl, uint8_t stream)
{
    xDMA_Base_TypeDef *base = hal_dma_get_base(ctrl);
    uint32_t it_flags = 0x3F;
    if (stream < 4) {
        uint32_t mask = it_flags << (stream * 6 + (stream >> 1) * 4);
        base->LIFCR |= mask;
    } else {
        stream -= 4;
        uint32_t mask = it_flags << (stream * 6 + (stream >> 1) * 4);
        base->HIFCR |= mask;
    }
}

dma_it_event_t hal_dma_get_it_event(dma_controller_t ctrl, uint8_t stream)
{
    xDMA_Base_TypeDef *base = hal_dma_get_base(ctrl);
    uint32_t it_event = 0;
    if (stream < 4) {
        it_event = base->LISR;
        it_event >>= (stream * 6 + (stream >> 1) * 4);
    } else {
        stream -= 4;
        it_event = base->HISR;
        it_event >>= (stream * 6 + (stream >> 1) * 4);
    }
    return it_event & 0x3F;
}

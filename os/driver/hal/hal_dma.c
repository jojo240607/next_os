//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_dma.h"
#include "../common/rcc.h"

/* 使能 DMA 时钟（系统初始化时可做一次） */
void dma_clock_enable(dma_controller_t ctrl)
{
    /* RCC AHB1ENR bit21 = DMA1, bit22 = DMA2 */
    if (ctrl == DMA_1) {
        rcc_periph_clock_enable(RCC_BUS_AHB1, xRCC_AHB1ENR_DMA1EN);
    } else {
        rcc_periph_clock_enable(RCC_BUS_AHB1, xRCC_AHB1ENR_DMA2EN);
    }
    __asm volatile ("dsb" ::: "memory");
}

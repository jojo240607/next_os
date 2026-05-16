//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_sdio.h"
#include "../common/rcc.h"

void hal_sdio_clock_enable() {
/* 1. 使能 SDIO 时钟 (APB2, bit 11) */
    rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB1ENR_SDIOEN);
}

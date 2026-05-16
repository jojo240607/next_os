//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_spi.h"
#include "../common/rcc.h"


xSPI_TypeDef* const SPIx[] = {
        (xSPI_TypeDef*)xSPI1_BASE,
        (xSPI_TypeDef*)xSPI2_BASE,
        (xSPI_TypeDef*)xSPI3_BASE
};


/* ---------- 时钟 ---------- */
void hal_spi_clock_enable(spi_id_t id)
{
    switch (id) {
        case SPI_1:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_SPI1EN);
            break;
        case SPI_2:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_SPI2EN);
            break;
        case SPI_3:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_SPI3EN);
            break;
        default:
            return;
    }
    __asm volatile ("dsb" ::: "memory");
}
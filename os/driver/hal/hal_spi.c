//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_spi.h"
#include "../common/rcc.h"
/* ---------- SPI 寄存器定义 (STM32F4) ---------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} xSPI_TypeDef;

static xSPI_TypeDef* const SPIx[] = {
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

void hal_spi_init(spi_id_t id, spi_mode_t mode, spi_frame_t frame_format, spi_br_t baudrate_div, spi_slave_mode_t master) {

    /* 3. 配置 SPI */
    xSPI_TypeDef *spi_ctrl = SPIx[id];

    uint32_t cr1 = 0;
    if (master) {
        cr1 |= (1 << 2);   // MSTR
    }
    cr1 |= (baudrate_div & 0x7) << 3;              // BR
    cr1 |= (mode & 0x3) << 0;                      // CPHA, CPOL
    cr1 |= (frame_format & 0x1) << 11;             // DFF
    cr1 |= (1 << 8) | (1 << 9);                         // SSI, SSM (软件 NSS)
    cr1 |= (1 << 6);                                    // SPE 最后使能
    spi_ctrl->CR1 = cr1;
}
void hal_spi_disable_it(spi_id_t id) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    spi_ctrl->CR2 = 0;
}
bool hal_spi_it_init(spi_id_t id, spi_it_t it_enable) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    
    if (it_enable) {
        uint32_t cr2 = spi_ctrl->CR2;
        if (it_enable & xSPI_IT_TXE) {
            cr2 |= xSPI_IE_TXE;   // TXEIE
        }
        if (it_enable & xSPI_IT_RXNE) {
            cr2 |= xSPI_IE_RXNE;   // RXNEIE
        }
        if (it_enable & xSPI_IT_ERR) {
            cr2 |= xSPI_IE_ERR;   // ERRIE
        }
        spi_ctrl->CR2 = cr2;
        return true;
    }
    return false;
}

void hal_spi_clear_it(spi_id_t id, spi_it_t it_event) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    spi_ctrl->CR2 &= ~it_event;
}

void hal_spi_set_it(spi_id_t id, spi_it_t it_event) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    spi_ctrl->CR2 |= it_event;
}

void hal_spi_dma_init(spi_id_t id, bool txdma, bool rxdma) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    if (txdma) {
        // 使能 USART 的 DMA 发送位 CR3 bit7 (DMAT)
        spi_ctrl->CR2 |= xSPI_CR2_TXDMAEN;
    }
    if (rxdma) {
        spi_ctrl->CR2 |= xSPI_CR2_RXDMAEN;
    }
}

uint32_t hal_spi_get_it_event(spi_id_t id) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    return spi_ctrl->SR;
}

void hal_spi_disable(spi_id_t id) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    spi_ctrl->CR1 &= ~SPI_CTL_SPE;   // 临时关 SPE
}

void hal_spi_enable(spi_id_t id) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    spi_ctrl->CR1 |= SPI_CTL_SPE;
}

volatile uint8_t *hal_spi_data_addr(spi_id_t id) {
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    return (volatile uint8_t*)&spi_ctrl->DR;
}
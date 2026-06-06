//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_spi.h"
#include "../common/rcc.h"
#include "../common/dma.h"
#include "../common/gpio.h"

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
// 软件 NSS (SSM=1, SSI=1)：CR1.SSM=1, CR1.SSI=1, CR2.SSOE 无效
// 硬件 NSS 输出 (SSM=0, SSOE=1)：CR1.SSM=0, CR2.SSOE=1, NSS 引脚自动翻转

void hal_spi_init(spi_id_t id, spi_mode_t mode, spi_frame_t frame_format, spi_br_t baudrate_div, spi_slave_mode_t master, spi_first_bit_t first_bit, spi_nss_mode_t nss_mode) {

    /* 3. 配置 SPI */
    xSPI_TypeDef *spi_ctrl = SPIx[id];

    uint32_t cr1 = 0;
    if (master) {
        cr1 |= (1 << 2);   // MSTR
    }
    cr1 |= (baudrate_div & 0x7) << 3;              // BR
    cr1 |= (mode & 0x3) << 0;                      // CPHA, CPOL
    cr1 |= (frame_format & 0x1) << 11;             // DFF
    cr1 |= ((uint32_t)(first_bit & 0x1)) << 7;     // LSBFIRST

    if (nss_mode == SPI_NSS_HARD) {
        /* 硬件 NSS 输出模式：SSM=0, SPI 自动翻转 NSS */
        // CR1.SSM=0 (bit 9=0), CR1.SSI 无效
        // CR2.SSOE=1 (bit 2=1) → 主模式下输出 NSS 信号
        spi_ctrl->CR2 = (1 << 2);  // SSOE #define SPI_CR2_SSOE_Pos            (2U)
    } else {
        /* 软件 NSS 模式：SSM=1, SSI=1, 内选始终有效 */
        cr1 |= (1 << 8) | (1 << 9);                // SSI + SSM
    }

    cr1 |= (1 << 6);                               // SPE
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

/* DMA 发送一个数据块
 * M2P 方向：dma_start_transfer(src, dst, len)
 *   → SxPAR=dst(外设=DR), SxM0AR=src(存储器=data)
 */
int hal_spi_send_dma(spi_id_t id, const dma_stream_config_t *dma_cfg, const uint8_t *data, uint16_t len)
{
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    dma_start_transfer(dma_cfg, (uint32_t)data, (uint32_t)(&spi_ctrl->DR), len);
    return 0;
}

/* DMA 接收（启动循环 DMA 到环形缓冲区）
 * P2M 方向：dma_start_transfer(src, dst, len)
 *   → SxPAR=src(外设=DR), SxM0AR=dst(存储器=buf)
 */
int hal_spi_recv_dma(spi_id_t id, const dma_stream_config_t *dma_cfg, uint8_t *buf, uint16_t buf_size)
{
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    dma_start_transfer(dma_cfg, (uint32_t)(&spi_ctrl->DR), (uint32_t)buf, buf_size);
    return 0;
}

// 发送 len 个字节，同时收下无用数据并丢弃
void hal_spi_transmit(spi_id_t id, const gpio_t* cs_pin, const uint8_t *tx_data, size_t len)
{
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    if (cs_pin) {
        gpio_reset(cs_pin);
    } else {
        spi_ctrl->CR1 |= SPI_CTL_SPE;
    }
    while (len--) {
        while (!(spi_ctrl->SR & xSPI_IT_TXE));      // 等待发送寄存器空
        spi_ctrl->DR = *tx_data++;          // 写入要发的字节
        while (!(spi_ctrl->SR & xSPI_IT_RXNE));     // 等待接收完成
        (void)spi_ctrl->DR;                 // 读取并丢弃接收到的字节
    }
    if (cs_pin) {
        gpio_set(cs_pin);
    } else {
        spi_ctrl->CR1 &= ~SPI_CTL_SPE;   // 临时关 SPE
    }
}

// 接收 len 个字节，同时发送 dummy 字节
void hal_spi_receive(spi_id_t id, const gpio_t* cs_pin, uint8_t *rx_data, size_t len)
{
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    if (cs_pin) {
        gpio_reset(cs_pin);
    } else {
        spi_ctrl->CR1 |= SPI_CTL_SPE;
    }
    while (len--) {
        while (!(spi_ctrl->SR & xSPI_IT_TXE));
        spi_ctrl->DR = 0xFF;                // 发送 dummy 数据（任意值）
        while (!(spi_ctrl->SR & xSPI_IT_RXNE));
        *rx_data++ = spi_ctrl->DR;          // 读出接收到的有效数据
    }
    if (cs_pin) {
        gpio_set(cs_pin);
    } else {
        spi_ctrl->CR1 &= ~SPI_CTL_SPE;   // 临时关 SPE
    }
}

// 同时收发 len 个字节
void hal_spi_transfer(spi_id_t id, const gpio_t* cs_pin, const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    xSPI_TypeDef *spi_ctrl = SPIx[id];
    if (cs_pin) {
        gpio_reset(cs_pin);
    } else {
        spi_ctrl->CR1 |= SPI_CTL_SPE;
    }
    while (len--) {
        while (!(spi_ctrl->SR & xSPI_IT_TXE));
        spi_ctrl->DR = tx_data ? *tx_data++ : 0xFF;  // 如果只收不发，可送 dummy
        while (!(spi_ctrl->SR & xSPI_IT_RXNE));
        uint8_t rx = spi_ctrl->DR;
        if (rx_data) *rx_data++ = rx;
    }
    if (cs_pin) {
        gpio_set(cs_pin);
    } else {
        spi_ctrl->CR1 &= ~SPI_CTL_SPE;   // 临时关 SPE
    }
}

//主机每发送一位数据，就会同时从从机接收一位数据，两者严格同步。
//int hal_spi_write(spi_id_t id, const uint8_t *buf, uint16_t buf_size, uint8_t *recv_buf, uint16_t recv_size) {
//    xSPI_TypeDef *spi_ctrl = SPIx[id];
//    if (recv_size < buf_size) {
//        return -1;
//    }
//    /* ─── 阻塞轮询模式 ─── */
//    for (uint16_t i = 0; i < buf_size; i++) {
//        while (!(spi_ctrl->SR & xSPI_IT_TXE));
//        spi_ctrl->DR = buf ? buf[i] : 0xFF;
//        while (!(spi_ctrl->SR & xSPI_IT_RXNE));
//        recv_buf[i] =  *(uint8_t*)&spi_ctrl->DR;
//    }
//
//}
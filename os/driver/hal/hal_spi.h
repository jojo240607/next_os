//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_SPI_H
#define STM32F4DISCOVERY_HAL_SPI_H
#include "stdint.h"

#define xSPI1_BASE  0x40013000UL
#define xSPI2_BASE  0x40003800UL
#define xSPI3_BASE  0x40003C00UL



/* SPI 内部 DMA 使能位 */
#define xSPI_CR2_TXDMAEN  (1 << 1)
#define xSPI_CR2_RXDMAEN  (1 << 0)

/* 状态标志 */
#define xSPI_SR_TXE   (1 << 1)
#define xSPI_SR_RXNE  (1 << 0)
#define xSPI_SR_BSY   (1 << 7)

/* 中断使能选择 */
//#define xSPI_IT_TXE   (1 << 0)   // 发送缓冲区空中断
//#define xSPI_IT_RXNE  (1 << 1)   // 接收缓冲区非空中断
//#define xSPI_IT_ERR   (1 << 2)   // 错误中断 (CRC/模式/溢出)

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
/* SPI 实例 */
typedef enum {
    SPI_1 = 0,
    SPI_2,
    SPI_3,
    SPI_MAX
} spi_id_t;

extern xSPI_TypeDef* const SPIx[];

void hal_spi_clock_enable(spi_id_t id);
#endif //STM32F4DISCOVERY_HAL_SPI_H

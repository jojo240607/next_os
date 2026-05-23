//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_SPI_H
#define STM32F4DISCOVERY_HAL_SPI_H

#include <stdbool.h>
#include "stdint.h"

#define xSPI1_BASE  0x40013000UL
#define xSPI2_BASE  0x40003800UL
#define xSPI3_BASE  0x40003C00UL



/* SPI 内部 DMA 使能位 */
#define xSPI_CR2_TXDMAEN  (1 << 1)
#define xSPI_CR2_RXDMAEN  (1 << 0)

/* 状态标志 */
//#define xSPI_SR_TXE   (1 << 1)
//#define xSPI_SR_RXNE  (1 << 0)
//#define xSPI_SR_BSY   (1 << 7)

/* 中断使能选择 */
//#define xSPI_IT_TXE   (1 << 0)   // 发送缓冲区空中断
//#define xSPI_IT_RXNE  (1 << 1)   // 接收缓冲区非空中断
//#define xSPI_IT_ERR   (1 << 2)   // 错误中断 (CRC/模式/溢出)


/* SPI 实例 */
typedef enum {
    SPI_1 = 0,
    SPI_2,
    SPI_3,
    SPI_MAX
} spi_id_t;

/* 模式：CPOL + CPHA */
typedef enum : uint8_t {
    SPI_MODE_0 = 0,     // CPOL=0, CPHA=0
    SPI_MODE_1,         // CPOL=0, CPHA=1
    SPI_MODE_2,         // CPOL=1, CPHA=0
    SPI_MODE_3,         // CPOL=1, CPHA=1
} spi_mode_t;

/* 数据帧格式 */
typedef enum :uint8_t {
    SPI_FRAME_8BIT = 0,
    SPI_FRAME_16BIT
} spi_frame_t;

/* 波特率分频 (fPCLK / 2^(br+1)) */
typedef enum : uint8_t {
    SPI_BR_DIV2 = 0,
    SPI_BR_DIV4,
    SPI_BR_DIV8,
    SPI_BR_DIV16,
    SPI_BR_DIV32,
    SPI_BR_DIV64,
    SPI_BR_DIV128,
    SPI_BR_DIV256,
} spi_br_t;

/* 主从模式 */
typedef enum : uint8_t {
    SPI_SLAVE = 0,
    SPI_MASTER = 1,
} spi_slave_mode_t;
typedef enum : uint8_t {
    xSPI_IT_NONE  = (0),
    xSPI_IT_TXE   = (1 << 1),   // SR.TXE (bit 1) → CR2.TXEIE (bit 7)
    xSPI_IT_RXNE  = (1 << 0),   // SR.RXNE (bit 0) → CR2.RXNEIE (bit 6)
    xSPI_IT_ERR   = (1 << 6) | (1 << 5) | (1 << 4),  // OVR (bit6) | MODF (bit5) | CRCERR (bit4)
    xSPI_IE_TXE  = (1 << 7),   // TXEIE
    xSPI_IE_RXNE = (1 << 6),   // RXNEIE
    xSPI_IE_ERR  = (1 << 5),   // ERRIE
} spi_it_t;

/* SPI 控制寄存器 (CR1) 位 */
typedef enum : uint16_t {
    SPI_CTL_SPE        = (1 << 6),   // SPI 使能
    SPI_CTL_CRCEN      = (1 << 13),  // CRC 使能
    SPI_CTL_BIDIMODE   = (1 << 15),  // 双向模式
    SPI_CTL_BIDIOE     = (1 << 14),  // 双向输出使能
    SPI_CTL_SSM        = (1 << 9),   // 软件片选管理
    SPI_CTL_SSI        = (1 << 8),   // 内部片选
    SPI_CTL_LSBFIRST   = (1 << 7),   // LSB 优先
} spi_ctl_t;


void hal_spi_clock_enable(spi_id_t id);
void hal_spi_init(spi_id_t id, spi_mode_t mode, spi_frame_t frame_format, spi_br_t baudrate_div, spi_slave_mode_t master);
void hal_spi_disable_it(spi_id_t id);
bool hal_spi_it_init(spi_id_t id, spi_it_t it_enable);
void hal_spi_set_it(spi_id_t id, spi_it_t it_event);
void hal_spi_clear_it(spi_id_t id, spi_it_t it_event);
void hal_spi_dma_init(spi_id_t id, bool txdma, bool rxdma);
uint32_t hal_spi_get_it_event(spi_id_t id);
void hal_spi_disable(spi_id_t id);
void hal_spi_enable(spi_id_t id);
volatile uint8_t *hal_spi_data_addr(spi_id_t id);

#endif //STM32F4DISCOVERY_HAL_SPI_H

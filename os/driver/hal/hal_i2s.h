//
// Created by zhiwei.gong on 2026/5/15.
//

#ifndef STM32F4DISCOVERY_HAL_I2S_H
#define STM32F4DISCOVERY_HAL_I2S_H
#include "stdint.h"
#include "stdbool.h"
#include "../common/dma.h"

/* ---------- SPI/I2S 寄存器基址 (复用 SPI2/3) ---------- */
#define xI2S2_BASE  0x40003800UL
#define xI2S3_BASE  0x40003C00UL
/* ---------- I2SCFGR 位 ---------- */
#define xI2S_I2SCFGR_CHLEN       (1 << 0)     // 通道长度 (每通道位数)
#define xI2S_I2SCFGR_DATLEN_Pos  1
#define xI2S_I2SCFGR_DATLEN_Msk  (3 << 1)     // 数据长度
#define xI2S_I2SCFGR_CKPOL       (1 << 3)     // 时钟极性
#define xI2S_I2SCFGR_I2SSTD_Pos  4
#define xI2S_I2SCFGR_I2SSTD_Msk  (3 << 4)     // I2S 标准
#define xI2S_I2SCFGR_PCMSYNC     (1 << 7)     // PCM 帧同步
#define xI2S_I2SCFGR_I2SCFG_Pos  8
#define xI2S_I2SCFGR_I2SCFG_Msk  (3 << 8)     // I2S 模式
#define xI2S_I2SCFGR_I2SE        (1 << 10)    // I2S 使能
#define xI2S_I2SCFGR_I2SMOD      (1 << 11)    // I2S 模式选择

/* ---------- I2SPR 位 ---------- */
#define xI2S_I2SPR_I2SDIV_Pos    0
#define xI2S_I2SPR_I2SDIV_Msk    (0xFF << 0)  // 分频系数
#define xI2S_I2SPR_ODD           (1 << 8)     // 奇因子
#define xI2S_I2SPR_MCKOE         (1 << 9)     // 主时钟输出使能

/* ---------- SR 位 ---------- */
#define xI2S_SR_TXE  (1 << 1)
#define xI2S_SR_RXNE (1 << 0)
#define xI2S_SR_BSY  (1 << 7)

/* ---------- CR2 位 ---------- */
#define xI2S_CR2_TXDMAEN  (1 << 1)
#define xI2S_CR2_RXDMAEN  (1 << 0)
#define xI2S_CR2_TXEIE    (1 << 7)
#define xI2S_CR2_RXNEIE   (1 << 6)
#define xI2S_CR2_ERRIE    (1 << 5)

/* ---------- SPI/I2S 寄存器结构 (仅列出 I2S 相关部分) ---------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;     // 0x1C - I2S 配置
    volatile uint32_t I2SPR;       // 0x20 - I2S 预分频
} xI2S_TypeDef;

/* I2S 实例 (与 SPI 共享硬件) */
typedef enum : uint8_t {
    I2S_2 = 0,   // 使用 SPI2/I2S2
    I2S_3 = 1,   // 使用 SPI3/I2S3
    I2S_MAX
} i2s_id_t;

/* 工作模式 */
typedef enum : uint8_t {
    xI2S_MODE_SLAVE_RX  = 0x00,   // 从机接收
    xI2S_MODE_SLAVE_TX  = 0x01,   // 从机发送
    xI2S_MODE_MASTER_RX = 0x02,   // 主机接收
    xI2S_MODE_MASTER_TX = 0x03    // 主机发送
} i2s_mode_t;

/* I2S 协议标准 */
typedef enum : uint8_t {
    xI2S_STANDARD_PHILIPS   = 0x00,   // I2S Philips 标准
    xI2S_STANDARD_MSB       = 0x01,   // MSB 对齐 (左对齐)
    xI2S_STANDARD_LSB       = 0x02,   // LSB 对齐 (右对齐)
    xI2S_STANDARD_PCM_SHORT = 0x03,   // PCM 短帧 (FS 1个CLK)
    xI2S_STANDARD_PCM_LONG  = 0x07    // PCM 长帧 (FS 13个CLK)
} i2s_standard_t;

/* 数据格式 (位宽) */
typedef enum : uint8_t {
    I2S_DATA_16BIT          = 0x00,
    I2S_DATA_16BIT_EXTENDED = 0x01,   // 16位数据扩展到32位帧
    I2S_DATA_24BIT          = 0x02,
    I2S_DATA_32BIT          = 0x03
} i2s_data_format_t;

/* 时钟极性 (空闲状态电平) */
typedef enum : uint8_t {
    I2S_CKPOL_LOW  = 0x00,
    I2S_CKPOL_HIGH = 0x01
} i2s_ckpol_t;

/* 中断使能选项 */
typedef enum :uint8_t {
    xI2S_IT_NONE = 0,
    xI2S_IT_TXE  = (1 << 0),
    xI2S_IT_RXNE = (1 << 1),
    xI2S_IT_ERR  = (1 << 2),
} i2s_it_t;


/* 回调事件 */
typedef enum : uint8_t {
    I2S_EVT_TX_HALF = 0,     // 半满
    I2S_EVT_TX_DONE = 1,     // 发送完成
    I2S_EVT_RX_HALF = 2,     // 半满
    I2S_EVT_RX_DONE = 3,     // 接收完成
    I2S_EVT_ERROR   = 4
} i2s_event_t;

/* 中断传输状态 (内部使用) */
typedef struct {
    const uint16_t *tx_buf;
    uint16_t       *rx_buf;
    uint16_t    total_len;
    uint16_t    tx_index;
    uint16_t    rx_index;
    bool        active;
    Semaphore * i2s_tx_sem;
    Semaphore * i2s_rx_sem;
} i2s_xfer_t;

extern xI2S_TypeDef* const I2Sx[I2S_MAX];

/* ── 硬件配置 ── */
void hal_i2s_clock_enable(i2s_id_t id);
void hal_i2s_config_pll(i2s_id_t id, uint32_t plln, uint32_t pllr);
void hal_i2s_config_i2spr(i2s_id_t id, uint8_t div, bool odd, bool mckoe);
void hal_i2s_config_i2scfgr(i2s_id_t id, i2s_mode_t mode, i2s_standard_t std,
                            uint8_t data_format, uint8_t clock_polarity, bool pcm_sync);
void hal_i2s_config_it(i2s_id_t id, uint8_t it_enable);
void hal_i2s_enable_txdma(i2s_id_t id, bool enable);
void hal_i2s_enable_rxdma(i2s_id_t id, bool enable);

/* ── 启动/停止 ── */
void hal_i2s_start(i2s_id_t id);
void hal_i2s_stop(i2s_id_t id);

/* ── 轮询传输 ── */
int hal_i2s_transmit(i2s_id_t id, const uint16_t *data, uint16_t len);
int hal_i2s_receive(i2s_id_t id, uint16_t *buffer, uint16_t len);

/* ── DMA 传输 ── */
int hal_i2s_transmit_dma(i2s_id_t id, const dma_stream_config_t *dma_cfg, const uint16_t *data, uint16_t len);
int hal_i2s_receive_dma(i2s_id_t id, const dma_stream_config_t *dma_cfg, uint16_t *buffer, uint16_t len);

/* ── 中断传输 ── */
int  hal_i2s_transmit_it(i2s_id_t id, i2s_xfer_t *xfer, const uint16_t *data, uint16_t len);
int  hal_i2s_receive_it(i2s_id_t id, i2s_xfer_t *xfer, uint16_t *buffer, uint16_t len);
void hal_i2s_abort_it(i2s_id_t id, i2s_xfer_t *xfer);
bool hal_i2s_is_busy(i2s_id_t id);

#endif //STM32F4DISCOVERY_HAL_I2S_H

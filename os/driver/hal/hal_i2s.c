//
// Created by zhiwei.gong on 2026/5/15.
//

#include <stddef.h>
#include "hal_i2s.h"
#include "../common/dma.h"
#include "../common/rcc.h"
#include "../../log/log.h"

xI2S_TypeDef* const I2Sx[I2S_MAX] = {
        (xI2S_TypeDef*)xI2S2_BASE,
        (xI2S_TypeDef*)xI2S3_BASE
};

/* ════ 硬件配置 ════ */
void hal_i2s_clock_enable(i2s_id_t id) {
    if (id == I2S_2)
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2S2EN);
    else
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2S3EN);
}

void hal_i2s_config_pll(i2s_id_t id, uint32_t plln, uint32_t pllr) {
    (void)id;
    volatile uint32_t *PLLI2SCFGR = (uint32_t*)0x40023884UL;
    *PLLI2SCFGR = (pllr & 0x7) << 28 | (plln & 0x1FF) << 6;
    volatile uint32_t *CR = (uint32_t*)0x40023800UL;
    *CR |= (1 << 26);
    while (!(*CR & (1 << 27)));
}

void hal_i2s_config_i2spr(i2s_id_t id, uint8_t div, bool odd, bool mckoe) {
    uint32_t v = (div & 0xFF) << 0;
    if (odd)   v |= xI2S_I2SPR_ODD;
    if (mckoe) v |= xI2S_I2SPR_MCKOE;
    I2Sx[id]->I2SPR = v;
}

void hal_i2s_config_i2scfgr(i2s_id_t id, i2s_mode_t mode, i2s_standard_t std,
                            uint8_t data_format, uint8_t clock_polarity, bool pcm_sync) {
    uint32_t v = xI2S_I2SCFGR_I2SMOD;
    v |= (mode & 0x3) << 8;
    v |= (std & 0x3) << 4;
    if (pcm_sync) v |= xI2S_I2SCFGR_PCMSYNC;
    if (clock_polarity) v |= (1 << 3);
    v |= (data_format & 0x3) << 1;
    if (data_format != I2S_DATA_16BIT) v |= xI2S_I2SCFGR_CHLEN;
    I2Sx[id]->I2SCFGR = v;
}

void hal_i2s_config_it(i2s_id_t id, uint8_t it_enable) {
    uint32_t cr2 = I2Sx[id]->CR2;
    if (it_enable & xI2S_IT_TXE)  cr2 |= xI2S_CR2_TXEIE;
    if (it_enable & xI2S_IT_RXNE) cr2 |= xI2S_CR2_RXNEIE;
    if (it_enable & xI2S_IT_ERR)  cr2 |= xI2S_CR2_ERRIE;
    I2Sx[id]->CR2 = cr2;
}

void hal_i2s_enable_txdma(i2s_id_t id, bool enable) {
    if (enable) I2Sx[id]->CR2 |= xI2S_CR2_TXDMAEN;
    else        I2Sx[id]->CR2 &= ~xI2S_CR2_TXDMAEN;
}

void hal_i2s_enable_rxdma(i2s_id_t id, bool enable) {
    if (enable) I2Sx[id]->CR2 |= xI2S_CR2_RXDMAEN;
    else        I2Sx[id]->CR2 &= ~xI2S_CR2_RXDMAEN;
}

/* ════ 启动/停止 ════ */
void hal_i2s_start(i2s_id_t id) {
    if (id >= I2S_MAX) return;
    I2Sx[id]->I2SCFGR |= xI2S_I2SCFGR_I2SE;
}

void hal_i2s_stop(i2s_id_t id) {
    if (id >= I2S_MAX) return;
    while (I2Sx[id]->SR & xI2S_SR_BSY);
    I2Sx[id]->I2SCFGR &= ~xI2S_I2SCFGR_I2SE;
}

/* ════ 轮询传输 ════ */
int hal_i2s_transmit(i2s_id_t id, const uint16_t *data, uint16_t len) {
    if (id >= I2S_MAX) return -1;
    xI2S_TypeDef *i2s = I2Sx[id];
    for (int i = 0; i < len; i++) {
        while (!(i2s->SR & xI2S_SR_TXE));
        i2s->DR = data[i];
    }
    return 0;
}

int hal_i2s_receive(i2s_id_t id, uint16_t *buffer, uint16_t len) {
    if (id >= I2S_MAX) return -1;
    xI2S_TypeDef *i2s = I2Sx[id];
    for (int i = 0; i < len; i++) {
        while (!(i2s->SR & xI2S_SR_RXNE));
        buffer[i] = i2s->DR;
    }
    return 0;
}

/* ════ DMA 传输 ════ */
int hal_i2s_transmit_dma(i2s_id_t id, const dma_stream_config_t *dma_cfg, const uint16_t *data, uint16_t len) {
    if (id >= I2S_MAX || dma_is_busy(dma_cfg)) return -1;
    return dma_start_transfer(dma_cfg, (uint32_t)data, (uint32_t)&I2Sx[id]->DR, len);
}

int hal_i2s_receive_dma(i2s_id_t id, const dma_stream_config_t *dma_cfg, uint16_t *buffer, uint16_t len) {
    if (id >= I2S_MAX || dma_is_busy(dma_cfg)) return -1;
    return dma_start_transfer(dma_cfg, (uint32_t)&I2Sx[id]->DR, (uint32_t)buffer, len);
}

bool hal_i2s_is_busy(i2s_id_t id) {
    if (id >= I2S_MAX) return false;
    return (I2Sx[id]->SR & xI2S_SR_BSY) != 0;
}

/* ════ 中断传输 ════ */
int hal_i2s_transmit_it(i2s_id_t id, i2s_xfer_t *xfer, const uint16_t *data, uint16_t len) {
    if (id >= I2S_MAX || len == 0 || !data || xfer->active) return -1;
    xfer->tx_buf = data; xfer->rx_buf = NULL;
    xfer->total_len = len; xfer->tx_index = 0; xfer->rx_index = 0;
    xfer->active = true;
    xI2S_TypeDef *i2s = I2Sx[id];
    if (!(i2s->I2SCFGR & xI2S_I2SCFGR_I2SE)) hal_i2s_start(id);
    i2s->DR = xfer->tx_buf ? xfer->tx_buf[xfer->tx_index] : 0xFF;
    xfer->tx_index++;
    i2s->CR2 |= xI2S_CR2_TXEIE;
    xfer->i2s_tx_sem->fun->take(xfer->i2s_tx_sem);
    return 0;
}

int hal_i2s_receive_it(i2s_id_t id, i2s_xfer_t *xfer, uint16_t *buffer, uint16_t len) {
    if (id >= I2S_MAX || len == 0 || !buffer || xfer->active) return -1;
    xfer->tx_buf = NULL; xfer->rx_buf = buffer;
    xfer->total_len = len; xfer->tx_index = 0; xfer->rx_index = 0;
    xfer->active = true;
    xI2S_TypeDef *i2s = I2Sx[id];
    i2s->DR = 0x0000;
    i2s->CR2 |= xI2S_CR2_RXNEIE | xI2S_CR2_TXEIE;
    xfer->i2s_rx_sem->fun->take(xfer->i2s_rx_sem);
    return 0;
}

void hal_i2s_abort_it(i2s_id_t id, i2s_xfer_t *xfer) {
    if (id >= I2S_MAX) return;
    xfer->active = false;
    I2Sx[id]->CR2 &= ~(xI2S_CR2_TXEIE | xI2S_CR2_RXNEIE);
}

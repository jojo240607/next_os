//
// Created by zhiwei.gong on 2026/5/15.
//

#include <stddef.h>
#include "hal_i2s.h"
#include "../common/dma.h"
#include "../../log/log.h"

xI2S_TypeDef* const I2Sx[I2S_MAX] = {
        (xI2S_TypeDef*)xI2S2_BASE,
        (xI2S_TypeDef*)xI2S3_BASE
};


/* ===================================================================
   启动/停止
   =================================================================== */
void hal_i2s_start(i2s_id_t id)
{
    if (id >= I2S_MAX) return;
    I2Sx[id]->I2SCFGR |= xI2S_I2SCFGR_I2SE;
}

void hal_i2s_stop(i2s_id_t id)
{
    if (id >= I2S_MAX) return;
    while (I2Sx[id]->SR & xI2S_SR_BSY);
    I2Sx[id]->I2SCFGR &= ~xI2S_I2SCFGR_I2SE;
}

/* ===================================================================
   轮询传输 (阻塞)
   =================================================================== */
int hal_i2s_transmit(i2s_id_t id, const uint16_t *data, uint16_t len)
{
    if (id >= I2S_MAX) return -1;
    xI2S_TypeDef *i2s = I2Sx[id];
    for (int i = 0; i < len; i++) {
        while (!(i2s->SR & xI2S_SR_TXE));
        i2s->DR = data[i];
    }
    return 0;
}

int hal_i2s_receive(i2s_id_t id, uint16_t *buffer, uint16_t len)
{
    if (id >= I2S_MAX) return -1;
    xI2S_TypeDef *i2s = I2Sx[id];
    for (int i = 0; i < len; i++) {
        while (!(i2s->SR & xI2S_SR_RXNE));
        buffer[i] = i2s->DR;
    }
    return 0;
}

/* ===================================================================
   DMA 传输
   =================================================================== */
int hal_i2s_transmit_dma(i2s_id_t id, const dma_stream_config_t *dma_cfg, const uint16_t *data, uint16_t len)
{
    if (id >= I2S_MAX) {
        return -1;
    }
    if (dma_is_busy(dma_cfg)) {
        return -1;
    }

    return dma_start_transfer(dma_cfg,
                              (uint32_t)data,
                              (uint32_t)&I2Sx[id]->DR, len);
}

int hal_i2s_receive_dma(i2s_id_t id, const dma_stream_config_t *dma_cfg, uint16_t *buffer, uint16_t len)
{
    if (id >= I2S_MAX) {
        return -1;
    }
    if (dma_is_busy(dma_cfg)) {
        return -1;
    }

    return dma_start_transfer(dma_cfg,
                              (uint32_t)&I2Sx[id]->DR,
                              (uint32_t)buffer, len);
}

bool hal_i2s_is_busy(i2s_id_t id)
{
    if (id >= I2S_MAX) return false;
    return (I2Sx[id]->SR & xI2S_SR_BSY) != 0;
}

//i2s_transmit_it 和 i2s_receive_it 都需要确保第一个数据被写入（或发送一个哑数据），并开启相应的中断
//I2S 是全双工，即使你只发不收，硬件也会收到数据，必须读取 DR 以防溢出。
//中断处理要同时照顾收发，用哑数据维持发送时钟（尤其在只接收时）。
//完成条件需要同时满足收发计数，避免过早关闭中断。
//如果需要高性能且避免中断风暴，推荐使用 DMA 代替中断，但对你的统一驱动框架来说，中断版本保持了与 SPI 相同的行为模式，便于移植和维护。
/**
 * @brief  启动中断发送 (非阻塞)
 * @param  id   I2S 实例
 * @param  data 待发送数据缓冲区 (16位)
 * @param  len  数据长度 (半字个数)
 * @return 0 成功, -1 忙碌或无效参数
 */
int hal_i2s_transmit_it(i2s_id_t id, i2s_xfer_t *xfer, const uint16_t *data, uint16_t len)
{
    if (id >= I2S_MAX || len == 0 || !data) {
        return -1;
    }
    if (xfer->active) {
        return -1;   // 上一次传输未结束
    }
    xfer->tx_buf    = data;
    xfer->rx_buf    = NULL;
    xfer->total_len = len;
    xfer->tx_index  = 0;
    xfer->rx_index  = 0;
    xfer->active    = true;

    xI2S_TypeDef *i2s = I2Sx[id];

    // 2. (在主模式下) 确保外设已启动。若未启动，则启动之
    if (!(i2s->I2SCFGR & xI2S_I2SCFGR_I2SE)) {
        hal_i2s_start(id);
    }

    // 3. ★ 关键步骤：先将第一个数据写入数据寄存器，以“触发”中断链 ★
    // 这样，当硬件将第一个数据从发送缓冲区转移到移位寄存器后，
    // TXE 标志位会再次置位并产生中断，后续的数据传输将由 ISR 接管。
    i2s->DR = xfer->tx_buf ? xfer->tx_buf[xfer->tx_index] : 0xFF;
    xfer->tx_index++;
    // 使能 TXE 中断（以及可选的 RXNE 中断，如果需要同时接收）
    i2s->CR2 |= xI2S_CR2_TXEIE;
    // 如果需要同时接收数据（全双工），也应使能 RXNEIE
    //Todo
    LOG_DEBUG("hal_i2s", "i2s tx sem take");
    xfer->i2s_tx_sem->fun->take(xfer->i2s_tx_sem);

    return 0;
}

/**
 * @brief  启动中断接收 (非阻塞)
 * @param  id     I2S 实例
 * @param  buffer 接收缓冲区 (16位)
 * @param  len    期望接收的数据长度
 * @return 0 成功, -1 忙碌或无效参数
 */
int hal_i2s_receive_it(i2s_id_t id, i2s_xfer_t *xfer, uint16_t *buffer, uint16_t len)
{
    if (id >= I2S_MAX || len == 0 || !buffer) {
        return -1;
    }
    if (xfer->active) {
        return -1;
    }

    xfer->tx_buf    = NULL;
    xfer->rx_buf    = buffer;
    xfer->total_len = len;
    xfer->tx_index  = 0;
    xfer->rx_index  = 0;
    xfer->active    = true;

    xI2S_TypeDef *i2s = I2Sx[id];
    // 发送一个哑数据以产生时钟（主模式必须，从模式可选）
    i2s->DR = 0x0000;
    // 使能RXNE 接收中断
    i2s->CR2 |= xI2S_CR2_RXNEIE;
    // 同时使能 TXE 中断，因为需要持续发送哑数据
    i2s->CR2 |= xI2S_CR2_TXEIE;
    xfer->i2s_rx_sem->fun->take(xfer->i2s_rx_sem);
    LOG_DEBUG("hal_i2s", "i2s recv take");
    return 0;
}

/**
 * @brief 中止当前中断传输
 */
void hal_i2s_abort_it(i2s_id_t id, i2s_xfer_t *xfer)
{
    if (id >= I2S_MAX) {
        return;
    }
    xfer->active = false;
    // 关闭相关中断
    I2Sx[id]->CR2 &= ~(xI2S_CR2_TXEIE | xI2S_CR2_RXNEIE);
}
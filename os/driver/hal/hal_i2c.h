//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_I2C_H
#define STM32F4DISCOVERY_HAL_I2C_H

#include <stdbool.h>
#include "stdint.h"
#include "../common/dma.h"

#define xI2C1_BASE  0x40005400UL
#define xI2C2_BASE  0x40005800UL
#define xI2C3_BASE  0x40005C00UL



/* 状态标志 */
//#define xI2C_SR1_TXE     (1 << 7)
//#define xI2C_SR1_RXNE    (1 << 6)
//#define xI2C_SR1_BTF     (1 << 2)
//#define xI2C_SR1_ADDR    (1 << 1)
//#define xI2C_SR1_SB      (1 << 0)
//#define xI2C_SR2_BUSY    (1 << 1)
//
///* CR1 控制位 */
//#define xI2C_CR1_PE      (1 << 0)
//#define xI2C_CR1_START   (1 << 8)
//#define xI2C_CR1_STOP    (1 << 9)
//#define xI2C_CR1_ACK     (1 << 10)
//#define xI2C_CR1_POS     (1 << 11)
//
///* CR2 中断使能位 */
//#define xI2C_CR2_ITEVTEN (1 << 9)
//#define xI2C_CR2_ITBUFEN (1 << 10)
//#define xI2C_CR2_ITERREN (1 << 8)



/* I2C 实例 */
typedef enum : uint8_t {
    I2C_1 = 0,
    I2C_2,
    I2C_3,
    I2C_MAX
} i2c_id_t;

/* 地址位数 */
typedef enum :uint8_t {
    I2C_ADDR_7BIT = 0,
    I2C_ADDR_10BIT
} i2c_addr_mode_t;

/* 中断使能选项 */
typedef enum :uint16_t {
    xI2C_IT_NONE = 0,
    xI2C_IT_TXE = (1 << 0),
    xI2C_IT_RXNE = (1 << 1),
    xI2C_IT_ERR = (1 << 2),   // 错误中断
    xI2C_IE_TXDMAEN   = (1 << 11),  // 使能发送 DMA (CR2.TXDMAEN)
    xI2C_IE_RXDMAEN   = (1 << 12),  // 使能接收 DMA (CR2.RXDMAEN)
    xI2C_IE_ITERREN   = (1 << 8),   // 错误中断使能
    xI2C_IE_ITEVTEN   = (1 << 9),   // 事件中断使能
    xI2C_IE_ITBUFEN   = (1 << 10),  // 缓冲区中断使能 (TXE/RXNE)
} i2c_it_t;

typedef enum : uint16_t {
    I2C_CTL_PE       = (1 << 0),   // 外设使能
    I2C_CTL_START    = (1 << 8),   // 生成起始条件
    I2C_CTL_STOP     = (1 << 9),   // 生成停止条件
    I2C_CTL_ACK      = (1 << 10),  // 应答使能
    I2C_CTL_POS      = (1 << 11),  // 应答位置 (用于双字节接收)
    I2C_CTL_SWRST    = (1 << 15),  // 软件复位 (位于 CR1)
} i2c_ctl_t;
typedef enum : uint16_t {
    I2C_FLG_SB       = (1 << 0),   // 起始条件已生成 (SR1)
    I2C_FLG_ADDR     = (1 << 1),   // 地址已发送 (SR1)
    I2C_FLG_BTF      = (1 << 2),   // 字节传输完成 (SR1)
    I2C_FLG_ADD10    = (1 << 3),   // 10位地址头已发送 (SR1)
    I2C_FLG_STOPF    = (1 << 4),   // 停止条件检测 (SR1)
    I2C_FLG_RXNE     = (1 << 6),   // 接收缓冲区非空 (SR1)
    I2C_FLG_TXE      = (1 << 7),   // 发送缓冲区空 (SR1)
    I2C_FLG_BERR     = (1 << 8),   // 总线错误 (SR1)
    I2C_FLG_ARLO     = (1 << 9),   // 仲裁丢失 (SR1)
    I2C_FLG_AF       = (1 << 10),  // 应答失败 (SR1)
    I2C_FLG_OVR      = (1 << 11),  // 过载/欠载 (SR1)
    I2C_FLG_BUSY     = (1 << 1),   // 总线忙 (SR2, bit1)
} i2c_flg_t;

void hal_i2c_clock_enable(i2c_id_t id);
void hal_i2c_set_clock(i2c_id_t id, uint32_t target_speed);
void hal_i2c_reset(i2c_id_t id);
void hal_i2c_set_addr(i2c_id_t id, i2c_addr_mode_t addr_mode, uint8_t own_address);
void hal_i2c_enable(i2c_id_t id);
bool hal_i2c_it_init(i2c_id_t id, i2c_it_t it_enable);
void hal_i2c_dma_init(i2c_id_t id, bool txdma, bool rxdma);
uint32_t hal_i2c_get_it_event(i2c_id_t id);
volatile uint8_t *hal_i2c_addr(i2c_id_t id);
void i2c_transmit(i2c_id_t id, uint8_t slave_addr, const uint8_t *data, uint16_t len);
void i2c_receive(i2c_id_t id, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
void hal_i2c_transmit_it_start(i2c_id_t id);
void hal_i2c_receive_it_start(i2c_id_t id);
void hal_i2c_clear_addr_flag(i2c_id_t id);
void hal_i2c_close_ack(i2c_id_t id);
void hal_i2c_stop(i2c_id_t id);
void hal_i2c_clear_it_event(i2c_id_t id, i2c_it_t it_event);
void i2c_transmit_dma(i2c_id_t id, const dma_stream_config_t *cfg, uint8_t slave_addr, const uint8_t *data, uint16_t len);

#endif //STM32F4DISCOVERY_HAL_I2C_H

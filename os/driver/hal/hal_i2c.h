//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_I2C_H
#define STM32F4DISCOVERY_HAL_I2C_H
#include "stdint.h"

#define xI2C1_BASE  0x40005400UL
#define xI2C2_BASE  0x40005800UL
#define xI2C3_BASE  0x40005C00UL



/* 状态标志 */
#define xI2C_SR1_TXE     (1 << 7)
#define xI2C_SR1_RXNE    (1 << 6)
#define xI2C_SR1_BTF     (1 << 2)
#define xI2C_SR1_ADDR    (1 << 1)
#define xI2C_SR1_SB      (1 << 0)
#define xI2C_SR2_BUSY    (1 << 1)

/* CR1 控制位 */
#define xI2C_CR1_PE      (1 << 0)
#define xI2C_CR1_START   (1 << 8)
#define xI2C_CR1_STOP    (1 << 9)
#define xI2C_CR1_ACK     (1 << 10)
#define xI2C_CR1_POS     (1 << 11)

/* CR2 中断使能位 */
#define xI2C_CR2_ITEVTEN (1 << 9)
#define xI2C_CR2_ITBUFEN (1 << 10)
#define xI2C_CR2_ITERREN (1 << 8)

/* ---------- I2C 寄存器定义 (STM32F4) ---------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
    volatile uint32_t FLTR;
} xI2C_TypeDef;

/* I2C 实例 */
typedef enum : uint8_t {
    I2C_1 = 0,
    I2C_2,
    I2C_3,
    I2C_MAX
} i2c_id_t;

extern xI2C_TypeDef* const I2Cx[];

void hal_i2c_clock_enable(i2c_id_t id);

#endif //STM32F4DISCOVERY_HAL_I2C_H

//
// Created by zhiwei.gong on 2026/5/18.
//

#include "hal_crc.h"

#include "../common/rcc.h"

/* CRC 寄存器定义 */
typedef struct {
    volatile uint32_t DR;    // 数据寄存器
    volatile uint32_t IDR;   // 独立数据寄存器
    volatile uint32_t CR;    // 控制寄存器
} xCRC_TypeDef;

#define xCRC_BASE  0x40023000UL
#define xCRC       ((xCRC_TypeDef *)xCRC_BASE)

/* CR 控制位 */
#define xCRC_CR_RESET   (1 << 0)
#define xCRC_CR_POLYSIZE_Pos  3
#define xCRC_CR_POLYSIZE_32   (0 << xCRC_CR_POLYSIZE_Pos)
#define xCRC_CR_POLYSIZE_16   (1 << xCRC_CR_POLYSIZE_Pos)
#define xCRC_CR_POLYSIZE_8    (2 << xCRC_CR_POLYSIZE_Pos)
#define xCRC_CR_REV_IN   (1 << 5)
#define xCRC_CR_REV_OUT  (1 << 7)

static bool initialized = false;

int crc_init(const crc_config_t *cfg)
{
    if (!cfg || initialized) return -1;

    /* 使能 CRC 时钟 (AHB1 bit12) */
    rcc_periph_clock_enable(RCC_BUS_AHB1, 12);

    /* 复位 CRC 单元 */
    xCRC->CR |= xCRC_CR_RESET;
    (void)xCRC->DR;  // 读一下确保复位完成

    /* 配置多项式长度 */
    uint32_t cr = 0;
    switch (cfg->poly_size) {
        case CRC_POLY_32: cr |= xCRC_CR_POLYSIZE_32; break;
        case CRC_POLY_16: cr |= xCRC_CR_POLYSIZE_16; break;
        case CRC_POLY_8:  cr |= xCRC_CR_POLYSIZE_8;  break;
    }
    if (cfg->reverse_in)  cr |= xCRC_CR_REV_IN;
    if (cfg->reverse_out) cr |= xCRC_CR_REV_OUT;

    xCRC->CR = cr;
    /* 设置初始值（通过 DR 写入？实际上需要通过多项式计算，这里简化用复位后的值） */
    xCRC->DR = cfg->init_value;  // 初始化数据寄存器

    initialized = true;
    return 0;
}

void crc_deinit(void)
{
    if (!initialized) return;
    xCRC->CR = xCRC_CR_RESET;
    initialized = false;
}

uint32_t crc_calculate(const uint8_t *data, uint32_t len)
{
    if (!initialized) return 0;
    /* 数据按字写入，这里简化按字节写入 */
    const uint32_t *p32 = (const uint32_t *)data;
    uint32_t words = len / 4;
    for (uint32_t i = 0; i < words; i++) {
        xCRC->DR = p32[i];
    }
    uint8_t rem = len % 4;
    if (rem) {
        uint32_t last_word = 0;
        for (int i = 0; i < rem; i++) {
            last_word |= data[words * 4 + i] << (i * 8);
        }
        xCRC->DR = last_word;
    }
    return xCRC->DR;
}

void crc_reset(void)
{
    xCRC->CR |= xCRC_CR_RESET;
}
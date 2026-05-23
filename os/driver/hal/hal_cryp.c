//
// Created by zhiwei.gong on 2026/5/18.
//

#include "hal_cryp.h"
#include "../common/rcc.h"
#include <string.h>

/* CRYP 寄存器定义 */
typedef struct {
    volatile uint32_t CR;     // 控制寄存器
    volatile uint32_t SR;     // 状态寄存器
    volatile uint32_t DI;     // 数据输入
    volatile uint32_t DO;     // 数据输出
    volatile uint32_t DMACR;  // DMA 控制
    volatile uint32_t IMCR;   // 中断屏蔽
    volatile uint32_t RISR;   // 原始中断状态
    volatile uint32_t MISR;   // 屏蔽后中断状态
    volatile uint32_t K[8];   // 密钥 (K0..K7)
    volatile uint32_t IV[4];  // 初始化向量 (IV0..IV3)
} xCRYP_TypeDef;

#define xCRYP_BASE  0x50060000UL
#define xCRYP       ((xCRYP_TypeDef *)xCRYP_BASE)

#define xCRYP_CR_ALGOMODE_Pos  3
#define xCRYP_CR_ALGODIR      (1 << 2)
#define xCRYP_CR_CRYPEN       (1 << 0)
#define xCRYP_SR_IFNF         (1 << 1)   // 输入 FIFO 非满
#define xCRYP_SR_OFNE         (1 << 0)   // 输出 FIFO 非空
#define xCRYP_SR_BUSY         (1 << 2)

static bool initialized = false;

/* 辅助：将字节数组转换为 32 位字数组写入密钥 */
static void cryp_set_key(const uint8_t *key, uint8_t key_size)
{
    uint32_t words = key_size / 4;
    for (uint32_t i = 0; i < words; i++) {
        uint32_t w = ((uint32_t)key[i*4+3] << 24) |
                     ((uint32_t)key[i*4+2] << 16) |
                     ((uint32_t)key[i*4+1] << 8)  |
                     ((uint32_t)key[i*4+0]);
        xCRYP->K[i] = w;
    }
}

/* 辅助：设置初始化向量 */
static void cryp_set_iv(const uint8_t *iv)
{
    for (int i = 0; i < 4; i++) {
        uint32_t w = ((uint32_t)iv[i*4+3] << 24) |
                     ((uint32_t)iv[i*4+2] << 16) |
                     ((uint32_t)iv[i*4+1] << 8)  |
                     ((uint32_t)iv[i*4+0]);
        xCRYP->IV[i] = w;
    }
}

int cryp_init(const cryp_config_t *cfg)
{
    if (!cfg || initialized) return -1;

    /* 使能 CRYP 时钟 (AHB2 bit4) */
    rcc_periph_clock_enable(RCC_BUS_AHB2, 4);

    /* 复位 CRYP */
    xCRYP->CR = 0;

    /* 配置算法和方向 */
    uint32_t cr = 0;
    cr |= (cfg->algo << xCRYP_CR_ALGOMODE_Pos);
    if (cfg->direction == CRYP_DIR_DECRYPT)
        cr |= xCRYP_CR_ALGODIR;

    /* 设置密钥 */
    uint8_t key_size = 128;   // 根据算法判定
    if (cfg->algo >= 4 && cfg->algo <= 7)  key_size = 192;
    else if (cfg->algo >= 8 && cfg->algo <= 11) key_size = 256;
    else if (cfg->algo >= 16) key_size = 64;  // DES / 3DES 简化
    cryp_set_key(cfg->key, key_size);

    /* 设置 IV（如果需要） */
    if (cfg->iv && (cfg->algo % 4 == 1 || cfg->algo % 4 == 2))  // CBC or CTR
        cryp_set_iv(cfg->iv);

    /* 使能 CRYP */
    xCRYP->CR = cr | xCRYP_CR_CRYPEN;

    initialized = true;
    return 0;
}

void cryp_deinit(void)
{
    if (!initialized) return;
    xCRYP->CR = 0;  // 关闭
    initialized = false;
}

int cryp_process(const uint8_t *input, uint8_t *output, uint32_t len)
{
    if (!initialized) return -1;

    // 按 32 位处理
    uint32_t words = len / 4;
    const uint32_t *in32 = (const uint32_t *)input;
    uint32_t *out32 = (uint32_t *)output;

    for (uint32_t i = 0; i < words; i++) {
        /* 等待输入 FIFO 可写 */
        while (!(xCRYP->SR & xCRYP_SR_IFNF));
        xCRYP->DI = in32[i];
        /* 等待输出 FIFO 可读 */
        while (!(xCRYP->SR & xCRYP_SR_OFNE));
        out32[i] = xCRYP->DO;
    }

    /* 处理剩余字节 */
    uint32_t rem = len % 4;
    if (rem) {
        uint32_t last_in = 0;
        memcpy(&last_in, &input[words*4], rem);
        while (!(xCRYP->SR & xCRYP_SR_IFNF));
        xCRYP->DI = last_in;
        while (!(xCRYP->SR & xCRYP_SR_OFNE));
        uint32_t last_out = xCRYP->DO;
        memcpy(&output[words*4], &last_out, rem);
    }
    return 0;
}

void cryp_enable_dma(bool tx, bool rx)
{
    uint32_t dma = 0;
    if (tx) dma |= (1 << 1);   // DMAINEN
    if (rx) dma |= (1 << 0);   // DMAOUTEN
    xCRYP->DMACR = dma;
}
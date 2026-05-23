//
// Created by zhiwei.gong on 2026/5/19.
//

#include "hal_hash.h"
#include "../common/rcc.h"
#include <string.h>

/* HASH 寄存器定义 */
typedef struct {
    volatile uint32_t CR;      // 控制寄存器
    volatile uint32_t DI;      // 数据输入
    volatile uint32_t SR;      // 状态寄存器
    uint32_t reserved[6];
    volatile uint32_t H[5];    // 哈希结果 (最多5个32位字)
    volatile uint32_t HR[5];   // 备用结果 (SHA-256 需要 8 个，这里简化)
} xHASH_TypeDef;

#define xHASH_BASE  0x50060400UL
#define xHASH       ((xHASH_TypeDef *)xHASH_BASE)

#define xHASH_CR_INIT   (1 << 2)   // 初始化哈希
#define xHASH_CR_DMAE   (1 << 3)   // DMA 使能
#define xHASH_CR_DATATYPE_Pos  4   // 数据类型 (0=32bit, 1=16bit, 2=8bit, 3=bit)
#define xHASH_CR_ALGO_Pos  18       // 算法选择 (0=SHA1, 1=SHA224, 2=SHA256, 3=MD5)
#define xHASH_SR_BUSY    (1 << 0)
#define xHASH_SR_DMAS    (1 << 1)   // DMA 状态

static bool initialized = false;
static uint8_t digest_size = 20;   // 根据算法变化

int hash_init(const hash_config_t *cfg)
{
    if (!cfg || initialized) return -1;

    /* 使能 HASH 时钟 (AHB2 bit5) */
    rcc_periph_clock_enable(RCC_BUS_AHB2, 5);

    /* 配置算法 */
    uint32_t cr = 0;
    cr |= (cfg->algo << xHASH_CR_ALGO_Pos);
    if (cfg->dma_mode) cr |= xHASH_CR_DMAE;
    xHASH->CR = cr;

    /* 设置摘要长度 */
    switch (cfg->algo) {
        case HASH_ALGO_SHA1:   digest_size = 20; break;
        case HASH_ALGO_SHA224: digest_size = 28; break;
        case HASH_ALGO_SHA256: digest_size = 32; break;
        case HASH_ALGO_MD5:    digest_size = 16; break;
    }

    initialized = true;
    return 0;
}

void hash_deinit(void)
{
    if (!initialized) return;
    xHASH->CR = 0;
    initialized = false;
}

/* 简单计算函数：阻塞模式 */
int hash_calculate(const uint8_t *data, uint32_t len, uint8_t *digest)
{
    if (!initialized) return -1;

    /* 初始化哈希 */
    xHASH->CR |= xHASH_CR_INIT;
    /* 写入数据（按字对齐） */
    const uint32_t *p32 = (const uint32_t *)data;
    uint32_t words = len / 4;
    for (uint32_t i = 0; i < words; i++) {
        xHASH->DI = p32[i];
        while (xHASH->SR & xHASH_SR_BUSY);  // 等待处理完成
    }
    /* 剩余字节处理（硬件支持字节填充，但需根据 DATATYPE 配置，这里简化） */

    /* 读取结果 */
    uint8_t *h = (uint8_t *)xHASH->H;
    memcpy(digest, h, digest_size);
    return 0;
}

void hash_start(void)
{
    xHASH->CR |= xHASH_CR_INIT;
}

void hash_update(const uint8_t *data, uint32_t len)
{
    const uint32_t *p32 = (const uint32_t *)data;
    uint32_t words = len / 4;
    for (uint32_t i = 0; i < words; i++) {
        xHASH->DI = p32[i];
        while (xHASH->SR & xHASH_SR_BUSY);
    }
}

void hash_final(uint8_t *digest)
{
    /* 硬件会自动填充，只需等待不忙，然后读取结果 */
    while (xHASH->SR & xHASH_SR_BUSY);
    memcpy(digest, (void *)xHASH->H, digest_size);
}
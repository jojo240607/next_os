#include "hal_rng.h"

/* RNG 寄存器定义 */
typedef struct {
    volatile uint32_t CR;   // 控制寄存器
    volatile uint32_t SR;   // 状态寄存器
    volatile uint32_t DR;   // 数据寄存器
} xRNG_TypeDef;

#define xRNG_BASE  0x50060800UL
#define xRNG       ((xRNG_TypeDef *)xRNG_BASE)

#define xRNG_CR_RNGEN   (1 << 2)
#define xRNG_SR_DRDY    (1 << 0)
#define xRNG_SR_CECS    (1 << 1)
#define xRNG_SR_SECS    (1 << 2)
#define xRNG_SR_CEIS    (1 << 5)
#define xRNG_SR_SEIS    (1 << 6)

static bool initialized = false;

int rng_init(const rng_config_t *cfg)
{
    (void)cfg;
    if (initialized) return -1;

    /* 使能 RNG 时钟 (AHB2 bit6) */
    rcc_periph_clock_enable(RCC_BUS_AHB2, 6);

    /* 使能 RNG */
    xRNG->CR |= xRNG_CR_RNGEN;

    initialized = true;
    return 0;
}

void rng_deinit(void)
{
    if (!initialized) return;
    xRNG->CR &= ~xRNG_CR_RNGEN;
    initialized = false;
}

uint32_t rng_get_random(void)
{
    if (!initialized) return 0;
    /* 等待数据就绪 */
    while (!(xRNG->SR & xRNG_SR_DRDY));
    return xRNG->DR;
}

bool rng_is_ready(void)
{
    return (xRNG->SR & xRNG_SR_DRDY) != 0;
}

bool rng_is_error(void)
{
    return (xRNG->SR & (xRNG_SR_CECS | xRNG_SR_SECS | xRNG_SR_CEIS | xRNG_SR_SEIS)) != 0;
}
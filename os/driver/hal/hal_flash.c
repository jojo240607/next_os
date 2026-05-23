//
// Created by zhiwei.gong on 2026/5/19.
//

#include "hal_flash.h"
#include <stddef.h>
#include "cmsis_gcc.h"

/* ───────── Flash 寄存器定义 ───────── */
typedef struct {
    volatile uint32_t ACR;       // 访问控制寄存器
    volatile uint32_t KEYR;      // 密钥寄存器
    volatile uint32_t OPTKEYR;   // 选项密钥寄存器
    volatile uint32_t SR;        // 状态寄存器
    volatile uint32_t CR;        // 控制寄存器
    volatile uint32_t OPTCR;     // 选项控制寄存器
} xFLASH_TypeDef;

#define xFLASH_BASE  0x40023C00UL
#define xFLASH       ((xFLASH_TypeDef *)xFLASH_BASE)

/* ACR 位定义 */
#define xFLASH_ACR_LATENCY_Pos   0
#define xFLASH_ACR_LATENCY_Msk   (0x07UL << xFLASH_ACR_LATENCY_Pos)
#define xFLASH_ACR_PRFTEN        (1UL << 8)   // 预取指使能
#define xFLASH_ACR_ICEN          (1UL << 9)   // 指令缓存使能
#define xFLASH_ACR_DCEN          (1UL << 10)  // 数据缓存使能

/* ───────── 内部状态 ───────── */
static bool flash_initialized = false;

/* ===================================================================
   初始化
   =================================================================== */
int hal_flash_init(const flash_config_t *cfg)
{
    if (!cfg || flash_initialized) return -1;

    uint32_t acr = xFLASH->ACR;

    /* 设置等待周期 */
    acr &= ~xFLASH_ACR_LATENCY_Msk;
    acr |= ((uint32_t)cfg->latency << xFLASH_ACR_LATENCY_Pos) & xFLASH_ACR_LATENCY_Msk;

    /* 预取指 */
    if (cfg->enable_prefetch)
        acr |= xFLASH_ACR_PRFTEN;
    else
        acr &= ~xFLASH_ACR_PRFTEN;

    /* 指令缓存 */
    if (cfg->enable_icache)
        acr |= xFLASH_ACR_ICEN;
    else
        acr &= ~xFLASH_ACR_ICEN;

    /* 数据缓存 */
    if (cfg->enable_dcache)
        acr |= xFLASH_ACR_DCEN;
    else
        acr &= ~xFLASH_ACR_DCEN;

    xFLASH->ACR = acr;
    __DSB();
    __ISB();

    flash_initialized = true;
    return 0;
}

void hal_flash_deinit(void)
{
    flash_initialized = false;
    /* 不恢复默认，Flash 配置通常不需要反初始化 */
}

/* ===================================================================
   修改等待周期
   =================================================================== */
int hal_flash_set_latency(flash_latency_t latency)
{
    if (!flash_initialized) return -1;

    uint32_t acr = xFLASH->ACR;
    acr &= ~xFLASH_ACR_LATENCY_Msk;
    acr |= ((uint32_t)latency << xFLASH_ACR_LATENCY_Pos) & xFLASH_ACR_LATENCY_Msk;
    xFLASH->ACR = acr;
    __DSB();
    __ISB();
    return 0;
}

/* ===================================================================
   预取指 / 缓存控制
   =================================================================== */
void hal_flash_enable_prefetch(bool enable)
{
    if (enable)
        xFLASH->ACR |= xFLASH_ACR_PRFTEN;
    else
        xFLASH->ACR &= ~xFLASH_ACR_PRFTEN;
}

void hal_flash_enable_icache(bool enable)
{
    if (enable)
        xFLASH->ACR |= xFLASH_ACR_ICEN;
    else
        xFLASH->ACR &= ~xFLASH_ACR_ICEN;
}

void hal_flash_enable_dcache(bool enable)
{
    if (enable)
        xFLASH->ACR |= xFLASH_ACR_DCEN;
    else
        xFLASH->ACR &= ~xFLASH_ACR_DCEN;
}
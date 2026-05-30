//
// Created by zhiwei.gong on 2026/5/18.
//

#include "hal_rcc.h"
#include "cmsis_gcc.h"
#include "hal_scb.h"

/* ---------- 内部静态变量：初始化后的真实频率 ---------- */
static uint32_t _sysclk = 16000000;  // 默认 HSI
static uint32_t _ahb_clk = 16000000;
static uint32_t _apb1_clk= 16000000;
static uint32_t _apb2_clk= 16000000;

/* ---------- 辅助：等待 HSE 就绪 ---------- */
static bool hal_wait_hse_ready(void)
{
    uint32_t timeout = 1000000;
    while (!(xRCC->CR & xRCC_CR_HSERDY)) {
        if (--timeout == 0) {
            return false;
        }
    }
    return true;
}

/* ---------- 辅助：等待 PLL 就绪 ---------- */
static bool hal_wait_pll_ready(void)
{
    uint32_t timeout = 1000000;
    while (!(xRCC->CR & xRCC_CR_PLLRDY)) {
        if (--timeout == 0) {
            return false;
        }
    }
    return true;
}

/* ---------- 系统时钟初始化 ---------- */
bool hal_rcc_sysclk_init(const rcc_sysclk_config_t *cfg)
{
    if (!cfg) return false;

    /* 1. 配置 HSE (如果使用) */
    if (cfg->sysclk_src == RCC_CLK_HSE || cfg->pll_src == RCC_PLLSRC_HSE) {
        if (cfg->hse_bypass) {
            xRCC->CR |= xRCC_CR_HSEBYP;
        } else {
            xRCC->CR &= ~xRCC_CR_HSEBYP;
        }
        xRCC->CR |= xRCC_CR_HSEON;
        if (!hal_wait_hse_ready()) {
            return false;
        }
    }

    /* 2. 配置 PLL (如果使用) */
    if (cfg->sysclk_src == RCC_CLK_PLL) {
        // 1. 检查当前系统时钟源是否是PLL
        //if ((xRCC->CFGR & 0x0C) == 0x08) {  // SWS = 10 = PLL
        //    // 2. 切换到HSI（系统时钟默认源，可靠）
        //    xRCC->CFGR &= ~0x03;             // SW = 00 (选择HSI)
        //    // 3. 等待HSI成为系统时钟
        //    while ((xRCC->CFGR & 0x0C) != 0x00); // SWS = 00 = HSI
        //}
        // 关闭 PLL 并等待
        //  xRCC->CR &= ~xRCC_CR_PLLON;
        //  while (xRCC->CR & xRCC_CR_PLLRDY);

        // 配置 PLL 参数
        uint32_t pllcfgr = 0;
        pllcfgr |= (cfg->pll_m & 0x3F) << 0;
        pllcfgr |= (cfg->pll_n & 0x1FF) << 6;

        // PLLP: 00=PLLP=2, 01=PLLP=4, 10=PLLP=6, 11=PLLP=8
        uint32_t p;
        if (cfg->pll_p == 2) {
            p = 0;
        } else if (cfg->pll_p == 4) {
            p = 1;
        } else if (cfg->pll_p == 6) {
            p = 2;
        } else {
            p = 3; // 8
        }
        pllcfgr |= (p & 0x03) << 16;
        pllcfgr |= (cfg->pll_q & 0x0F) << 24;
        pllcfgr |= (cfg->pll_src & 0x01) << 22;

        xRCC->PLLCFGR = pllcfgr;

        // 开启 PLL
        xRCC->CR |= xRCC_CR_PLLON;
        if (!hal_wait_pll_ready()) {
            return false;
        }
    }

    /* 3. 配置总线分频 (先不分频以便切换系统时钟时稳定) */
    /* 3. 构造完整的 CFGR 值 (SW + 分频系数) 并一次性写入 */
    uint32_t cfgr = 0;

    /* 设置 SW */
    switch (cfg->sysclk_src) {
        case RCC_CLK_HSI: cfgr |= 0x00; break;
        case RCC_CLK_HSE: cfgr |= 0x01; break;
        case RCC_CLK_PLL: cfgr |= 0x02; break;
    }

    /* 设置总线分频 */
    cfgr |= (cfg->ahb_div  & 0x0F) << 4;
    cfgr |= (cfg->apb1_div & 0x07) << 10;
    cfgr |= (cfg->apb2_div & 0x07) << 13;

    /* 一次性写入 CFGR */
    xRCC->CFGR = cfgr;
    __DSB();

/* 等待切换完成 */
    uint32_t sws_target = (cfgr & 0x03) << 2;
    while ((xRCC->CFGR & (0x03 << 2)) != sws_target);

/* 3. 等待切换完成 */
    //while ((xRCC->CFGR & 0x0C) != sws_target);

    /* 5. 如果之前使用了 HSI 且不再需要，可关闭 HSI 以省电（可选） */

    /* 6. 计算真实频率并保存 */
    uint32_t src_freq;
    if (cfg->sysclk_src == RCC_CLK_HSI) {
        src_freq = cfg->hsi_freq;
    } else if (cfg->sysclk_src == RCC_CLK_HSE) {
        src_freq = cfg->hse_freq;
    } else { // PLL
        uint32_t plli = (cfg->pll_src == RCC_PLLSRC_HSE) ? cfg->hse_freq : cfg->hsi_freq;
        src_freq = plli / cfg->pll_m * cfg->pll_n / cfg->pll_p;
    }
    _sysclk = src_freq;

    // 计算 AHB/APB1/APB2
    // AHB 分频：HCLK = SYSCLK / AHB_DIV (具体值参考手册)
    uint32_t ahb_div_val;
    switch (cfg->ahb_div) {
        case RCC_AHB_DIV1:   ahb_div_val = 1;   break;
        case RCC_AHB_DIV2:   ahb_div_val = 2;   break;
        case RCC_AHB_DIV4:   ahb_div_val = 4;   break;
        case RCC_AHB_DIV8:   ahb_div_val = 8;   break;
        case RCC_AHB_DIV16:  ahb_div_val = 16;  break;
        case RCC_AHB_DIV64:  ahb_div_val = 64;  break;
        case RCC_AHB_DIV128: ahb_div_val = 128; break;
        case RCC_AHB_DIV256: ahb_div_val = 256; break;
        case RCC_AHB_DIV512: ahb_div_val = 512; break;
        default: ahb_div_val = 1;                break;
    }
    _ahb_clk = _sysclk / ahb_div_val;

    // APB1 分频
    uint32_t apb1_div_val = (cfg->apb1_div == RCC_APB_DIV1) ? 1 :
                            (cfg->apb1_div == RCC_APB_DIV2) ? 2 :
                            (cfg->apb1_div == RCC_APB_DIV4) ? 4 :
                            (cfg->apb1_div == RCC_APB_DIV8) ? 8 :
                            16;
    _apb1_clk = _ahb_clk / apb1_div_val;
    // APB2 分频
    uint32_t apb2_div_val = (cfg->apb2_div == RCC_APB_DIV1) ? 1 :
                            (cfg->apb2_div == RCC_APB_DIV2) ? 2 :
                            (cfg->apb2_div == RCC_APB_DIV4) ? 4 :
                            (cfg->apb2_div == RCC_APB_DIV8) ? 8 :
                            16;
    _apb2_clk = _ahb_clk / apb2_div_val;

    return true;
}




/* ---------- 外设时钟使能/禁用 ---------- */
static volatile uint32_t* hal_rcc_get_enr_ptr(rcc_bus_t bus)
{
    switch (bus) {
        case RCC_BUS_AHB1:
            return &xRCC->AHB1ENR;
        case RCC_BUS_AHB2:
            return &xRCC->AHB2ENR;
        case RCC_BUS_AHB3:
            return &xRCC->AHB3ENR;
        case RCC_BUS_APB1:
            return &xRCC->APB1ENR;
        case RCC_BUS_APB2:
            return &xRCC->APB2ENR;
        default:
            return NULL;
    }
}

void hal_rcc_periph_clock_enable(rcc_bus_t bus, rcc_enter_t periph_bit)
{
    volatile uint32_t *reg = hal_rcc_get_enr_ptr(bus);
    if (reg) {
        *reg |= (1U << periph_bit);
    }
}

void hal_rcc_periph_clock_disable(rcc_bus_t bus, rcc_enter_t periph_bit)
{
    volatile uint32_t *reg = hal_rcc_get_enr_ptr(bus);
    if (reg) {
        *reg &= ~(1U << periph_bit);
    }
}

/* ---------- 外设复位 ---------- */
static volatile uint32_t* hal_rcc_get_rstr_ptr(rcc_bus_t bus)
{
    switch (bus) {
        case RCC_BUS_AHB1:
            return &xRCC->AHB1RSTR;
        case RCC_BUS_AHB2:
            return &xRCC->AHB2RSTR;
        case RCC_BUS_AHB3:
            return &xRCC->AHB3RSTR;
        case RCC_BUS_APB1:
            return &xRCC->APB1RSTR;
        case RCC_BUS_APB2:
            return &xRCC->APB2RSTR;
        default:
            return NULL;
    }
}

void hal_rcc_periph_reset_assert(rcc_bus_t bus, rcc_enter_t periph_bit)
{
    volatile uint32_t *reg = hal_rcc_get_rstr_ptr(bus);
    if (reg) {
        *reg |= (1U << periph_bit);
    }
}

void hal_rcc_periph_reset_deassert(rcc_bus_t bus, rcc_enter_t periph_bit)
{
    volatile uint32_t *reg = hal_rcc_get_rstr_ptr(bus);
    if (reg) {
        *reg &= ~(1U << periph_bit);
    }
}

/* ---------- 系统复位 ---------- */
void hal_rcc_system_reset(void)
{
    // 写 SCB 的 AIRCR 寄存器请求系统复位 (这里直接操作 SCB)
    xSCB->AIRCR = xSCB_AIRCR_VECTKEY | xSCB_AIRCR_SYSRESETREQ;
    __asm volatile ("dsb");
    while (1);
}

/* ---------- 频率查询 ---------- */
uint32_t hal_rcc_get_system_clock(void) {
    return _sysclk;
}
uint32_t hal_rcc_get_ahb_clock(void) {
    return _ahb_clk;
}
uint32_t hal_rcc_get_apb1_clock(void) {
    return _apb1_clk;
}
uint32_t hal_rcc_get_apb2_clock(void) {
    return _apb2_clk;
}

uint32_t rcc_get_timer_clock(rcc_bus_t bus) {
    uint32_t pclk, pre;
    if (bus == RCC_BUS_APB1) {
        pclk = hal_rcc_get_apb1_clock();
        pre  = (xRCC->CFGR >> 10) & 0x7;   // APB1 PRE
    } else {
        pclk = hal_rcc_get_apb2_clock();
        pre  = (xRCC->CFGR >> 13) & 0x7;   // APB2 PRE
    }
    // 如果预分频系数不等于 1，定时器时钟 = 2 x APB 时钟
    if (pre == 0)       // 分频系数 = 1
        return pclk;
    else
        return pclk * 2;
}

//
// Created by zhiwei.gong on 2026/5/19.
//

#include "hal_scb.h"
#include "cmsis_gcc.h"


//volatile xSCB_TypeDef *xSCB = (volatile xSCB_TypeDef *)xSCB_BASE;
/* ───────── 内部状态 ───────── */
static bool scb_initialized = false;
//static scb_config_t current_cfg;

/* ===================================================================
   初始化
   =================================================================== */
int hal_scb_init(const scb_config_t *cfg)
{
    if (!cfg || scb_initialized) {
        return -1;
    }

    /* 1. 设置优先级分组 */
    hal_scb_set_priority_group(cfg->priority_group);

    /* 2. 设置向量表基地址 */
    hal_scb_set_vector_table(cfg->vector_table_base);

    /* 3. 故障捕获控制 */
    if (cfg->enable_fault_trap) {
        xSCB->CCR |= xSCB_CCR_DIV_0_TRP | xSCB_CCR_UNALIGN_TRP;
    } else {
        xSCB->CCR &= ~(xSCB_CCR_DIV_0_TRP | xSCB_CCR_UNALIGN_TRP);
    }

    /* 4. 睡眠模式控制 */
    if (cfg->enable_sleep_on_exit) {
        xSCB->SCR |= xSCB_SCR_SLEEPONEXIT;
    } else {
        xSCB->SCR &= ~xSCB_SCR_SLEEPONEXIT;
    }
    hal_scb_enable_deep_sleep(cfg->enable_deep_sleep);

    //current_cfg = *cfg;
    scb_initialized = true;
    return 0;
}

void hal_scb_deinit(void)
{
    scb_initialized = false;
    /* 不强制恢复默认，仅标记未初始化 */
}

/* ===================================================================
   系统复位
   =================================================================== */
void hal_scb_system_reset(void)
{
    xSCB->AIRCR = xSCB_AIRCR_VECTKEY | xSCB_AIRCR_SYSRESETREQ;
    __DSB();
    while (1) {
        __WFE();
    }
}

/* ===================================================================
   向量表偏移
   =================================================================== */
void hal_scb_set_vector_table(uint32_t base)
{
    xSCB->VTOR = base;
    __DSB();
}

/* ===================================================================
   优先级分组
   =================================================================== */
void hal_scb_set_priority_group(scb_priority_group_t group)
{
    uint32_t aircr = xSCB->AIRCR;
    aircr &= ~(xSCB_AIRCR_VECTKEY | xSCB_AIRCR_PRIGROUP_Msk);
    aircr |= xSCB_AIRCR_VECTKEY | ((uint32_t)group << xSCB_AIRCR_PRIGROUP_Pos);
    xSCB->AIRCR = aircr;
    __DSB();
    __ISB();
}

/* ===================================================================
   故障状态
   =================================================================== */
uint32_t hal_scb_get_fault_status(void)
{
    /* CFSR 包含 MMFSR, BFSR, UFSR */
    return xSCB->CFSR;
}

void hal_scb_clear_fault_status(void)
{
    /* 写 1 清除 CFSR 中的 MMFSR/BFSR/UFSR */
    xSCB->CFSR = 0xFFFFFFFF;
    /* HFSR 中的 FORCED 位写 1 可清除，但通常需要先清除底层故障 */
    xSCB->HFSR = xSCB->HFSR;  /* 写回原值来清除有效位 */
}

/* ===================================================================
   睡眠模式
   =================================================================== */
void hal_scb_set_sleep_mode(scb_sleep_mode_t mode)
{
    if (mode == SCB_SLEEP_ON_EXIT) {
        xSCB->SCR |= xSCB_SCR_SLEEPONEXIT;
    } else {
        xSCB->SCR &= ~xSCB_SCR_SLEEPONEXIT;
    }
}

void hal_scb_enable_deep_sleep(bool enable)
{
    if (enable) {
        xSCB->SCR |= xSCB_SCR_SLEEPDEEP;
    } else {
        xSCB->SCR &= ~xSCB_SCR_SLEEPDEEP;
    }
}

/* ===================================================================
   系统信息查询
   =================================================================== */
uint32_t hal_scb_get_cpuid(void)
{
    return xSCB->CPUID;
}

uint32_t hal_scb_get_vect_active(void)
{
    return (xSCB->ICSR & 0x1FF);  // VECTACTIVE[8:0]
}

bool hal_scb_is_in_exception(void)
{
    return (xSCB->ICSR & (0x1FF)) != 0;
}
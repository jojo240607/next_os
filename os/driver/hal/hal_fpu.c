//
// Created by zhiwei.gong on 2026/5/15.
//

#include "hal_fpu.h"
#include "cmsis_gcc.h"

/* ────────────────────────────────
   系统初始化
   ──────────────────────────────── */
int hal_fpu_init(const fpu_config_t *cfg)
{
    if (!cfg) return -1;

    /* 配置 FPU 访问权限 */
    if (cfg->mode == FPU_MODE_DISABLED) {
        /* 禁止 CP10/CP11 */
        xSCB_CPACR &= ~(xCPACR_CP10_FULL | xCPACR_CP11_FULL);
    } else {
        /* 使能完全访问 */
        xSCB_CPACR |= xCPACR_CP10_FULL | xCPACR_CP11_FULL;
        __DSB();
        __ISB();
    }

    /* 配置上下文控制 */
    uint32_t fpccr = xSCB_FPCCR;
    if (cfg->enable_auto_state_preservation)
        fpccr |= xFPCCR_ASPEN;
    else
        fpccr &= ~xFPCCR_ASPEN;

    if (cfg->enable_lazy_stacking)
        fpccr |= xFPCCR_LSPEN;
    else
        fpccr &= ~xFPCCR_LSPEN;

    xSCB_FPCCR = fpccr;
    __DSB();
    __ISB();

    //fpu_current_cfg = *cfg;
   // fpu_initialized = true;
    return 0;
}

void hal_fpu_deinit(void)
{
    xSCB_CPACR &= ~(xCPACR_CP10_FULL | xCPACR_CP11_FULL);
    //fpu_initialized = false;
}
void hal_fpu_disable(void)
{
    xSCB_CPACR &= ~(xCPACR_CP10_FULL | xCPACR_CP11_FULL);
    __DSB();
    __ISB();
}

/* ────────────────────────────────
   运行时使能/禁止
   ──────────────────────────────── */
void hal_fpu_enable(fpu_mode_t mode)
{
    if (mode == FPU_MODE_DISABLED) {
        hal_fpu_disable();
        return;
    }
    xSCB_CPACR |= xCPACR_CP10_FULL | xCPACR_CP11_FULL;
    __DSB();
    __ISB();
}

/* ────────────────────────────────
   状态查询
   ──────────────────────────────── */
bool hal_fpu_is_enabled(void)
{
    return (xSCB_CPACR & (xCPACR_CP10_FULL | xCPACR_CP11_FULL)) ==
           (xCPACR_CP10_FULL | xCPACR_CP11_FULL);
}

bool hal_fpu_is_lazy_stacking_active(void)
{
    return (xSCB_FPCCR & xFPCCR_LSPEN) != 0;
}

/* ────────────────────────────────
   异常处理 (通过 FPU 中断)
   ──────────────────────────────── */
//void hal_fpu_register_exception_handler(fpu_exception_handler_t handler)
//{
//    exception_callback = handler;
//}
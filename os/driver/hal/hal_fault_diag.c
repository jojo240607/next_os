//
// Created by zhiwei.gong on 2026/5/20.
//

#include "hal_fault_diag.h"
#include "hal_fault_diag.h"
#include <string.h>
#include <stdio.h>   // 仅用于 snprintf，若不需要格式化可移除
#include "cmsis_gcc.h"

/* ──────── 内部全局配置 ──────── */
static bool initialized = false;

/* ===================================================================
   初始化（可选）
   =================================================================== */
int hal_fault_diag_init()
{
    if (initialized) {
        return -1;
    }
    initialized = true;
    return 0;
}

/* ===================================================================
   故障解码：读取 SCB 寄存器并填充 fault_info_t
   =================================================================== */
void hal_fault_diag_decode(fault_info_t *info)
{
    if (!info) return;

    memset(info, 0, sizeof(*info));

    uint32_t cfsr = xSCB->CFSR;
    uint32_t hfsr = xSCB->HFSR;
    uint32_t mmfar = xSCB->MMFAR;
    uint32_t bfar  = xSCB->BFAR;
    uint32_t fpscr;

    // 读取 FPSCR
    __ASM volatile ("VMRS %0, FPSCR" : "=r" (fpscr));

    // 拆分为子状态
    uint8_t mmfsr = (cfsr >> 0)  & 0xFF;
    uint8_t bfsr  = (cfsr >> 8)  & 0xFF;
    uint16_t ufsr  = (cfsr >> 16) & 0xFFFF;

    /* ─── 确定故障类型 ─── */
    if (hfsr & (1 << 30)) {             // FORCED 位
        info->forced_hardfault = true;
        info->type = FAULT_TYPE_HARD_FAULT;
    }

    /* 根据子状态进一步细分 */
    if (ufsr) {
        info->type = FAULT_TYPE_USAGE_FAULT;
        info->ufsr_nocp        = (ufsr >> 1) & 1;
        info->ufsr_unaligned   = (ufsr >> 8) & 1;
        info->ufsr_div_by_zero = (ufsr >> 9) & 1;
        info->ufsr_inv_state   = (ufsr >> 3) & 1;
        info->ufsr_inv_pc      = (ufsr >> 2) & 1;
    }
    if (bfsr) {
        info->type = FAULT_TYPE_BUS_FAULT;
        info->bfsr_precise   = (bfsr >> 7) & 1;   // BFARVALID
        info->bfsr_imprecise = (bfsr >> 6) & 1;
        if (info->bfsr_precise) info->bfar = bfar;
    }
    if (mmfsr) {
        info->type = FAULT_TYPE_MEM_MANAGE;
        info->mmfsr_access_violation = (mmfsr >> 3) & 1;
        info->mmfsr_invalid_region   = (mmfsr >> 4) & 1;
        if (mmfsr & (1 << 7))        // MMFARVALID
            info->mmfar = mmfar;
    }

    /* ─── 浮点状态 ─── */
    if (fpscr & 0x1F) {
        info->fp_exceptions = (fp_exception_t)(fpscr & 0x1F);
    }
    info->fp_disabled = (ufsr >> 1) & 1;   // NOCP 位

    /* 如果未找到具体子故障，但 HFSR 已置 FORCED，则保留为 HardFault */
    if (info->type == FAULT_TYPE_NONE && (hfsr & (1 << 30)))
        info->type = FAULT_TYPE_HARD_FAULT;
}

/* ===================================================================
   格式化输出（简洁版，不依赖 printf 也可以直接用）
   这里用 snprintf 方便，若不想用可自行拼接
   =================================================================== */
int hal_fault_diag_snprint(char *buf, size_t size, const fault_info_t *info)
{
    if (!buf || !info) return -1;

    int n = 0;
    n = snprintf(buf, size, "Fault: ");
    if (info->forced_hardfault) n += snprintf(buf + n, size - n, "FORCED ");
    switch (info->type) {
        case FAULT_TYPE_USAGE_FAULT:  n += snprintf(buf + n, size - n, "UsageFault "); break;
        case FAULT_TYPE_BUS_FAULT:    n += snprintf(buf + n, size - n, "BusFault "); break;
        case FAULT_TYPE_MEM_MANAGE:   n += snprintf(buf + n, size - n, "MemManage "); break;
        case FAULT_TYPE_HARD_FAULT:   n += snprintf(buf + n, size - n, "HardFault "); break;
        default: break;
    }

    if (info->ufsr_nocp)        n += snprintf(buf + n, size - n, "[FPU off]");
    if (info->ufsr_unaligned)   n += snprintf(buf + n, size - n, "[Unaligned]");
    if (info->ufsr_div_by_zero) n += snprintf(buf + n, size - n, "[Div0]");
    if (info->bfsr_precise)     n += snprintf(buf + n, size - n, "[BFAR=0x%08lX]", info->bfar);
    if (info->mmfar)            n += snprintf(buf + n, size - n, "[MMFAR=0x%08lX]", info->mmfar);
    if (info->fp_exceptions)    n += snprintf(buf + n, size - n, "[FP:0x%02X]", info->fp_exceptions);

    return n;
}
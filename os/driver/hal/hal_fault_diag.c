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


/* ---- 全局变量：保存所有诊断信息，可在调试器中查看 ---- */
volatile uint32_t fault_pc;          // 故障时的 PC
volatile uint32_t fault_lr;          // 故障时的 LR（栈帧中的）
volatile uint32_t fault_exc_return;  // 进入 HardFault 时的 EXC_RETURN

volatile uint32_t fault_msp;         // 故障瞬间的 MSP 值
volatile uint32_t fault_psp;         // 故障瞬间的 PSP 值

volatile uint32_t fault_cfsr;        // 配置故障状态寄存器
volatile uint32_t fault_hfsr;        // HardFault 状态寄存器
volatile uint32_t fault_mmfar;       // MemManage 故障地址
volatile uint32_t fault_bfar;        // BusFault 故障地址

volatile ExceptionStackFrame *fault_frame;  // 指向栈帧，可以观察原始 r0~r3
volatile uint32_t *fault_stack_start;       // 当前堆栈的起始地址，方便观察栈内容

/* ---- 诊断函数的核心 ---- */
__attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile(
            "MOV    R0, LR                \n"   // 把 LR (EXC_RETURN) 传给 C 函数
            "MOV    R1, SP                \n"   // 把当前 SP 传给 C 函数
            "BL     HardFault_Diagnosis   \n"
            );
}


void HardFault_Diagnosis(uint32_t exc_return, uint32_t sp) {
    // 1. 保存 EXC_RETURN，用于判断进入异常前的栈和模式
    fault_exc_return = exc_return;

    // 2. 根据 exc_return 的 bit2 判断原栈是 MSP 还是 PSP
    if ((exc_return & 0x4) == 0) {
        // 异常前使用的是主栈 (MSP)
        fault_msp = sp;
        fault_psp = __get_PSP();   // PSP 可能仍是旧值，仅供参考
    } else {
        // 异常前使用的是进程栈 (PSP)
        fault_psp = sp;
        fault_msp = __get_MSP();
    }

    // 3. 获取栈帧指针（8 个寄存器的快照）
    fault_frame = (ExceptionStackFrame *)sp;
    fault_pc = fault_frame->pc;
    fault_lr = fault_frame->lr;

    // 4. 读取所有故障状态寄存器
    fault_cfsr  = xSCB->CFSR;   // 高 16 位 UFSR，中 8 位 BFSR，低 8 位 MFSR
    fault_hfsr  = xSCB->HFSR;
    fault_mmfar = xSCB->MMFAR;
    fault_bfar  = xSCB->BFAR;

    // 5. (可选) 记录当前栈的起始地址，方便在 Memory 窗口中查看栈内容
    //    这里简单地取 sp 为起点，你可以上下调整偏移量
    fault_stack_start = (uint32_t *)sp;

    // 6. 【关键】在这里设置一个断点（或死循环），让调试器停住
    __BKPT(0);  // 如果调试器不支持，可以换成 while(1);
    // while(1);
}
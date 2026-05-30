//
// Created by zhiwei.gong on 2026/5/20.
//

#ifndef STM32F4DISCOVERY_FAULT_DIAG_H
#define STM32F4DISCOVERY_FAULT_DIAG_H
#include <stdint.h>
#include <stdbool.h>
#include "stddef.h"
#include "hal_scb.h"            // 你的 SCB 结构体定义

/* ───────── 故障类型枚举 ───────── */
typedef enum {
    FAULT_TYPE_NONE              = 0,
    FAULT_TYPE_HARD_FAULT        = 1,
    FAULT_TYPE_MEM_MANAGE        = 2,
    FAULT_TYPE_BUS_FAULT         = 3,
    FAULT_TYPE_USAGE_FAULT       = 4,
} fault_type_t;

/* ───────── 浮点异常枚举 ───────── */
typedef enum {
    FP_EXCEPTION_NONE     = 0,
    FP_EXCEPTION_INVALID  = (1 << 0),   // 无效操作
    FP_EXCEPTION_DIVZERO  = (1 << 1),   // 除零
    FP_EXCEPTION_OVERFLOW = (1 << 2),   // 上溢
    FP_EXCEPTION_UNDERFLOW= (1 << 3),   // 下溢
    FP_EXCEPTION_INEXACT  = (1 << 4),   // 不精确
} fp_exception_t;
/* 故障栈帧结构体 (Cortex-M4 进入异常时自动压栈的顺序) */
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;         // 被压入栈的原始 LR
    uint32_t pc;         // 故障指令地址
    uint32_t xpsr;
} ExceptionStackFrame;

/* ───────── 故障详情结构体 ───────── */
typedef struct {
    /* 顶层故障类型 */
    fault_type_t type;

    /* 是否由其他故障强制升级 (HFSR.FORCED) */
    bool forced_hardfault;

    /* UsageFault 具体原因 */
    bool ufsr_nocp;             // 未使能协处理器 (FPU)
    bool ufsr_unaligned;        // 非对齐访问
    bool ufsr_div_by_zero;      // 除零
    bool ufsr_inv_state;        // 无效状态
    bool ufsr_inv_pc;           // 无效 PC 加载

    /* BusFault 具体原因 */
    bool bfsr_precise;          // 精确总线错误 (BFAR 有效)
    bool bfsr_imprecise;        // 不精确总线错误

    /* MemManage 具体原因 */
    bool mmfsr_access_violation; // 访问违规
    bool mmfsr_invalid_region;   // 非法区域

    /* 故障地址 (若有效) */
    uint32_t mmfar;             // 存储器管理故障地址
    uint32_t bfar;              // 总线故障地址

    /* 浮点状态 */
    fp_exception_t fp_exceptions;   // FPSCR 异常标志组合
    bool fp_disabled;               // FPU 未使能 (与 ufsr_nocp 关联)

} fault_info_t;


/* ========== API ========== */
int  hal_fault_diag_init();
void hal_fault_diag_decode(fault_info_t *info);

/* 辅助：将故障信息格式化为简短字符串（用于日志） */
int  hal_fault_diag_snprint(const fault_info_t *info);

#endif //STM32F4DISCOVERY_FAULT_DIAG_H

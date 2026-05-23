//
// Created by zhiwei.gong on 2026/5/15.
//

#ifndef STM32F4DISCOVERY_HAL_FPU_H
#define STM32F4DISCOVERY_HAL_FPU_H
#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "hal_scb.h"

/* ───────── FPU 硬件访问宏 (协处理器访问指令) ───────── */
/* CPACR 寄存器 (位于 SCB) */
#define xSCB_CPACR_ADDR     0xE000ED88UL
#define xSCB_CPACR          (*(volatile uint32_t *)xSCB_CPACR_ADDR)

/* FPCCR 寄存器 (浮点上下文控制寄存器) */
#define xSCB_FPCCR_ADDR     0xE000EF34UL
#define xSCB_FPCCR          (*(volatile uint32_t *)xSCB_FPCCR_ADDR)

/* FPCA (浮点上下文活动) 位, 在 CONTROL 寄存器中 */
#define xFPU_ACTIVE_BIT     2

/* CPACR 位: CP10 和 CP11 的访问权 */
#define xCPACR_CP10_FULL    (0x03U << 20)  /* 完全访问 CP10 */
#define xCPACR_CP11_FULL    (0x03U << 22)  /* 完全访问 CP11 */

/* FPCCR 位 */
#define xFPCCR_LSPEN        (1U << 30)     /* 惰性堆栈使能 */
#define xFPCCR_ASPEN        (1U << 31)     /* 自动状态保存使能 */


/* FPU 工作模式 */
typedef enum :uint8_t {
    FPU_MODE_DISABLED        = 0,      /* FPU 未使能 */
    FPU_MODE_PRIVILEGED_ONLY = 1,      /* 仅特权线程可使用 FPU */
    FPU_MODE_FULL_ACCESS     = 2       /* 特权+用户线程均可使用 FPU */
} fpu_mode_t;

/* FPU 配置描述符 */
typedef struct {
    fpu_mode_t          mode;              /* 工作模式 */
    bool                enable_lazy_stacking; /* 使能惰性堆栈 */
    bool                enable_auto_state_preservation; /* 使能自动状态保存 */
} fpu_config_t;


/* ========== API ========== */
int hal_fpu_init(const fpu_config_t *cfg);
void hal_fpu_deinit(void);

/* FPU 使能/禁止 */
void hal_fpu_enable(fpu_mode_t mode);
void hal_fpu_disable(void);

/* 状态查询 */
bool hal_fpu_is_enabled(void);
bool hal_fpu_is_lazy_stacking_active(void);

#endif //STM32F4DISCOVERY_HAL_FPU_H

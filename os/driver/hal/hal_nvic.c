//
// Created by zhiwei.gong on 2026/5/19.
//

#include "hal_nvic.h"
#include <stddef.h>
//#include "stm32f4xx.h"
#include "cmsis_gcc.h"
#include "../../log/log.h"

/* ───────── NVIC 寄存器结构 (Cortex-M4) ───────── */
typedef struct {
    volatile uint32_t ISER[8];       // 0x000 中断使能寄存器 (8个×32位=256位)
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];       // 0x080 中断清除使能寄存器
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];       // 0x100 中断挂起置位寄存器
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];       // 0x180 中断挂起清除寄存器
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];       // 0x200 中断活动位寄存器
    uint32_t RESERVED4[56];
    volatile uint8_t  IP[240];       // 0x300 中断优先级寄存器 (字节访问)
    uint32_t RESERVED5[644];
    volatile uint32_t STIR;          // 0xE00 软件触发中断寄存器
} xNVIC_TypeDef;

#define xNVIC_BASE  0xE000E100UL
#define xNVIC       ((xNVIC_TypeDef *)xNVIC_BASE)


/* ───────── 内部状态 ───────── */
static bool nvic_initialized = false;
static scb_priority_group_t current_group = SCB_PRIORITY_GROUP_4;

/* ===================================================================
   初始化
   =================================================================== */
int hal_nvic_init(const nvic_config_t *cfg)
{
    if (!cfg || nvic_initialized) return -1;

    /* 1. 设置全局优先级分组 */
    hal_nvic_set_priority_group(cfg->priority_group);

    /* 2. 批量配置中断 */
    if (*cfg->irq_configs && cfg->num_irqs > 0) {
        for (uint32_t i = 0; i < cfg->num_irqs; i++) {
            const nvic_irq_config_t *irq_cfg = cfg->irq_configs[i];
            hal_nvic_set_priority(irq_cfg->irq,
                              irq_cfg->preempt_priority,
                              irq_cfg->sub_priority);
            if (irq_cfg->enable) {
                hal_nvic_enable_irq(irq_cfg->irq);
            }
        }
    }

    nvic_initialized = true;
    return 0;
}

void hal_nvic_deinit(void)
{
    /* 禁用所有外部中断 */
    for (int i = 0; i < 8; i++) {
        xNVIC->ICER[i] = 0xFFFFFFFF;
    }
    nvic_initialized = false;
}

/* ===================================================================
   优先级分组
   =================================================================== */
void hal_nvic_set_priority_group(scb_priority_group_t group)
{
    uint32_t aircr = xSCB->AIRCR;
    aircr &= ~(xSCB_AIRCR_VECTKEY | xSCB_AIRCR_PRIGROUP_Msk);
    aircr |= xSCB_AIRCR_VECTKEY | ((uint32_t)group << xSCB_AIRCR_PRIGROUP_Pos);
    xSCB->AIRCR = aircr;
    __DSB();
    __ISB();
    current_group = group;
}

/* ===================================================================
   使能 / 禁止中断
   =================================================================== */
void hal_nvic_enable_irq(nvic_irqn_t irq)
{
    if (irq >= 0) {
        __ASM volatile("":::"memory");
        xNVIC->ISER[(uint32_t)irq >> 5] = (1UL << ((uint32_t)irq & 0x1F));
        __ASM volatile("":::"memory");
    } else {
        if (irq == xMemoryManagement_IRQn) {
            xSCB->SHCSR |= xSCB_SHCSR_MEMFAULTENA_Msk;
        } else if (irq == xBusFault_IRQn) {
            xSCB->SHCSR |= xSCB_SHCSR_BUSFAULTENA_Msk;
        } else if (irq == xUsageFault_IRQn) {
            xSCB->SHCSR |= xSCB_SHCSR_USGFAULTENA_Msk;
        }
    }
}

void hal_nvic_disable_irq(nvic_irqn_t irq)
{
    if (irq >= 0) {
        __ASM volatile("":::"memory");
        xNVIC->ICER[(uint32_t)irq >> 5] = (1UL << ((uint32_t)irq & 0x1F));
        __ASM volatile("":::"memory");
    }
}

bool hal_nvic_is_enabled(nvic_irqn_t irq)
{
    if (irq >= 0) {
        return (xNVIC->ISER[(uint32_t)irq >> 5] & (1UL << ((uint32_t)irq & 0x1F))) != 0;
    }
    return false;
}

static inline void hal_nvic_setpriority_line(nvic_irqn_t irq, uint32_t priority)
{
    if (irq >= 0)
    {
        xNVIC->IP[((uint32_t)irq)]               = (uint8_t)((priority << (0x07 - current_group)) & (uint32_t)0xFFUL);
    }
    else
    {
        xSCB->SHP[(((uint32_t)irq) & 0xFUL)-4UL] = (uint8_t)((priority << (0x07 - current_group)) & (uint32_t)0xFFUL);
    }
}

static inline void write_shp(int irq, uint8_t priority) {
    uint32_t idx = (((uint32_t)irq) & 0xFUL) - 4UL;
    *(volatile uint8_t *)(0xE000ED18 + idx) = (uint8_t)(priority << 4);
}

/* ===================================================================
   优先级配置
   寄存器所属模块	常见访问宽度要求	安全操作方式
    SCB (SHP, SHCSR 等)	字节 / 字混合，SHP 必须字节	用 NVIC_* 函数
    MPU (RBAR, RASR)	字 (32 bit)	CMSIS 结构体，直接 32 位赋值
    NVIC (ISER, ICER, IP)	字 / 字节混合，IP 必须字节	NVIC_SetPriority / NVIC_EnableIRQ
    外设 (USART, SPI 等)	视寄存器定义	用 __IO 类型指针，匹配位宽
    SysTick	字	CMSIS 函数或 32 位赋值
   =================================================================== */
void hal_nvic_set_priority(nvic_irqn_t irq, nvic_priority_t preempt_priority, uint8_t sub_priority)
{
    /* 根据优先级分组计算实际写入的优先级值 */
    uint32_t priority_group = (uint32_t) current_group;
    uint32_t preempt_bits = (0x07 - priority_group);  // 抢占优先级占用的位数
    uint32_t sub_bits = (0x04 - preempt_bits);         // 子优先级占用的位数
    /* 确保不超出位宽 */
    preempt_priority &= (1 << preempt_bits) - 1;
    sub_priority &= (1 << sub_bits) - 1;
    /* 硬件优先级值 = (抢占优先级 << sub_bits) | 子优先级, 然后放到高4位 */
    uint8_t priority = ((preempt_priority << sub_bits) | sub_priority) << 4;
    if (irq < 0) {
        //SCB->SHP是uint8_t数组，共12个元素，对应系统异常-12到-1。
        xSCB->SHP[(((uint32_t)irq) & 0xFUL)-4UL] = priority;
    } else {
        xNVIC->IP[(uint32_t) irq] = priority;
    }
    LOG_DEBUG("nvic", "init irq %d set_priority %d", irq, preempt_priority);
}

/* ===================================================================
   挂起 / 清除挂起
   =================================================================== */
void hal_nvic_set_pending(nvic_irqn_t irq)
{
    if (irq >= 0) {
        xNVIC->ISPR[(uint32_t)irq >> 5] = (1UL << ((uint32_t)irq & 0x1F));
    }
}

void hal_nvic_clear_pending(nvic_irqn_t irq)
{
    if (irq >= 0) {
        xNVIC->ICPR[(uint32_t)irq >> 5] = (1UL << ((uint32_t)irq & 0x1F));
    }
}

/* ===================================================================
   查询活动状态
   =================================================================== */
uint32_t hal_nvic_get_active(nvic_irqn_t irq)
{
    if (irq >= 0) {
        return (xNVIC->IABR[(uint32_t)irq >> 5] >> ((uint32_t)irq & 0x1F)) & 0x01;
    }
    return 0;
}

/* ===================================================================
   全局中断控制
   =================================================================== */
void hal_nvic_global_irq_enable(void)
{
    __ASM volatile ("CPSIE I" ::: "memory");
}

void hal_nvic_global_irq_disable(void)
{
    __ASM volatile ("CPSID I" ::: "memory");
}

/* ===================================================================
   系统复位
   =================================================================== */
void hal_nvic_system_reset(void)
{
    xSCB->AIRCR = xSCB_AIRCR_VECTKEY | xSCB_AIRCR_SYSRESETREQ;
    __DSB();
    while (1) {
        __WFE();
    }
}
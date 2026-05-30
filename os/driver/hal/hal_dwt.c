//
// Created by zhiwei.gong on 2026/5/21.
//

#include "hal_dwt.h"
#include <stddef.h>
#include "hal_rcc.h"

/* ── DWT 寄存器结构体 ── */
typedef struct {
    volatile uint32_t CTRL;      /* 控制寄存器 */
    volatile uint32_t CYCCNT;    /* 周期计数器 */
    volatile uint32_t CPICNT;    /* CPI 计数器 */
    volatile uint32_t EXCCNT;    /* 异常计数器 */
    volatile uint32_t SLEEPCNT;  /* 睡眠计数器 */
    volatile uint32_t LSUCNT;    /* LSU 计数器 */
    volatile uint32_t FOLDCNT;   /* 折叠计数器 */
    volatile const uint32_t PCSR; /* 程序计数器采样寄存器 */
    /* 之后是比较器相关寄存器 (这里省略) */
} xDWT_TypeDef;

#define xDWT_BASE   0xE0001000UL
#define xDWT        ((xDWT_TypeDef *)xDWT_BASE)

/* ── CoreDebug 寄存器 (DEMCR) ── */
typedef struct {
    volatile uint32_t DHCSR;
    volatile uint32_t DCRSR;
    volatile uint32_t DCRDR;
    volatile uint32_t DEMCR;
} xCoreDebug_TypeDef;

#define xCoreDebug_BASE  0xE000EDF0UL
#define xCoreDebug       ((xCoreDebug_TypeDef *)xCoreDebug_BASE)

/* DEMCR 位 */
#define xCoreDebug_DEMCR_TRCENA  (1UL << 24)   /* 全局 DWT 使能 */

/* DWT_CTRL 位 */
#define xDWT_CTRL_NOCYCCNT   (1UL << 25)       /* 禁止周期计数器 (写 1 禁止) */
#define xDWT_CTRL_CYCCNTENA  (1UL << 0)        /* 使能周期计数器 */

/* ── 内部状态 ── */
static uint64_t overflow_count = 0;          /* CYCCNT 溢出次数 (32位) */
static uint32_t last_ticks = 0;              /* 上一次读取的 tick 值，用于溢出检测 */

/* ── 系统时间戳 (微秒) 累计 ── */
static sys_time_t time_stamp = {0};            /* 累计微秒数，低32位对应部分周期 */


/* ===================================================================
   初始化
   =================================================================== */
int hal_dwt_init()
{

    /* 1. 使能 DWT 跟踪 (通过 CoreDebug->DEMCR) */
    xCoreDebug->DEMCR |= xCoreDebug_DEMCR_TRCENA;

    /* 2. 复位周期计数器 */
    xDWT->CYCCNT = 0;

    /* 3. 清除可能已存在的禁止标志 */
    if (ENABLE_CYCLE_COUNTER) {
        xDWT->CTRL &= ~xDWT_CTRL_NOCYCCNT;      /* 清除禁止位，允许计数 */
        xDWT->CTRL |= xDWT_CTRL_CYCCNTENA;      /* 使能 CYCCNT */
    } else {
        xDWT->CTRL |= xDWT_CTRL_NOCYCCNT;
    }

    /* 4. 初始化溢出跟踪变量 */
    last_ticks = 0;
    overflow_count = 0;
    time_stamp.time_us = 0;
    return 0;
}

void hal_dwt_deinit(void)
{
    xDWT->CTRL |= xDWT_CTRL_NOCYCCNT;           /* 禁止周期计数器 */
    xCoreDebug->DEMCR &= ~xCoreDebug_DEMCR_TRCENA;
}

/* ===================================================================
   获取当前周期计数值 (处理溢出)
   =================================================================== */
uint32_t hal_dwt_get_ticks(void)
{
    uint32_t current = xDWT->CYCCNT;
    if (current < last_ticks) {
        /* 发生 32 位溢出 */
        overflow_count++;
    }
    last_ticks = current;
    return current;
}

/* ===================================================================
   微秒/毫秒延时
   =================================================================== */
void hal_dwt_delay_us(uint32_t us)
{
    uint32_t start = hal_dwt_get_ticks();
    uint32_t ticks_needed = ((uint64_t)us * hal_rcc_get_system_clock()) / 1000000UL;
    while ((uint32_t)(hal_dwt_get_ticks() - start) < ticks_needed) {
        /* 等待 */
    }
}

void hal_dwt_delay_ms(uint32_t ms)
{
    uint32_t start = hal_dwt_get_ticks();
    uint32_t ticks_needed = ((uint64_t)ms * hal_rcc_get_system_clock()) / 1000UL;
    while ((uint32_t)(hal_dwt_get_ticks() - start) < ticks_needed) {
        /* 等待 */
    }
}

/* ===================================================================
   测量两个时刻之间的微秒数
   =================================================================== */
uint32_t hal_dwt_elapsed_us(uint32_t start_ticks)
{
    uint32_t end = hal_dwt_get_ticks();
    uint32_t delta = end - start_ticks;
    return (uint32_t)(((uint64_t)delta * 1000000) / hal_rcc_get_system_clock());
}

/* ===================================================================
   获取系统时间戳 (微秒)，累计 64 位
   =================================================================== */
sys_time_t *hal_dwt_get_timestamp_us(void)
{
    uint32_t current = xDWT->CYCCNT;

    /* 检测 32 位溢出 */
    uint32_t delta;
    if (current >= last_ticks) {
        delta = current - last_ticks;
    } else {
        delta = (0xFFFFFFFF - last_ticks) + current + 1;
        overflow_count++;
    }
    last_ticks = current;

    /* 把 delta 转换为微秒，累加到总时间戳 */
    uint64_t us_delta = ((uint64_t)delta * 1000000ULL) / hal_rcc_get_system_clock();
    time_stamp.time_us += us_delta;

    return &time_stamp;
}
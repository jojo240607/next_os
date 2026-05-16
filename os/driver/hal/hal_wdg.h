//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_WDG_H
#define STM32F4DISCOVERY_HAL_WDG_H
#include "stdint.h"

#define xWWDG_BASE  0x40002C00UL
#define xWWDG       ((xWWDG_TypeDef *)xWWDG_BASE)

/* ---------- WWDG 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t CR;       // 0x00 控制寄存器 (位0..6: T[6:0], 位7: WDGA)
    volatile uint32_t CFR;      // 0x04 配置寄存器 (位0..6: W[6:0], 位8..9: WDGTB, 位9: EWI)
    volatile uint32_t SR;       // 0x08 状态寄存器 (位0: EWIF)
} xWWDG_TypeDef;



#define xIWDG_BASE  0x40003000UL
#define xIWDG       ((xIWDG_TypeDef *)xIWDG_BASE)

/* ---------- IWDG 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t KR;       // 0x00 关键字寄存器
    volatile uint32_t PR;       // 0x04 预分频寄存器
    volatile uint32_t RLR;      // 0x08 重装载寄存器
    volatile uint32_t SR;       // 0x0C 状态寄存器
} xIWDG_TypeDef;


/* ---------- RCC 控制/状态寄存器 (用于复位标志) ---------- */
typedef struct {
    volatile uint32_t CSR;
} xRCC_CSR_TypeDef;


#define xRCC_CSR_BASE  0x40023874UL
#define xRCC_CSR       ((xRCC_CSR_TypeDef *)xRCC_CSR_BASE)

#define xRCC_CSR_IWDGRSTF   (1 << 29)
#define xRCC_CSR_WWDGRSTF   (1 << 30)
#define xRCC_CSR_RMVF       (1 << 24)

/* ---------- IWDG 关键字定义 ---------- */
#define xIWDG_KEY_ENABLE    0xCCCC   // 启动看门狗
#define xIWDG_KEY_RELOAD    0xAAAA   // 喂狗 (重装载)
#define xIWDG_KEY_UNLOCK    0x5555   // 解锁 PR 和 RLR 寄存器
#define xIWDG_KEY_LOCK      0x0000   // 锁定 PR 和 RLR (写其他值即锁定)


#endif //STM32F4DISCOVERY_HAL_WDG_H

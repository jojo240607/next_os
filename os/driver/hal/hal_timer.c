//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_timer.h"
#include "../common/rcc.h"


xTIM_TypeDef* const TIMx[TIM_MAX] = {
        (xTIM_TypeDef*)xTIM1_BASE,
        (xTIM_TypeDef*)xTIM2_BASE,
        (xTIM_TypeDef*)xTIM3_BASE,
        (xTIM_TypeDef*)xTIM4_BASE,
        (xTIM_TypeDef*)xTIM5_BASE,
        (xTIM_TypeDef*)xTIM6_BASE,
        (xTIM_TypeDef*)xTIM7_BASE,
        (xTIM_TypeDef*)xTIM8_BASE,
        (xTIM_TypeDef*)xTIM9_BASE,
        (xTIM_TypeDef*)xTIM10_BASE,
        (xTIM_TypeDef*)xTIM11_BASE,
        (xTIM_TypeDef*)xTIM12_BASE,
        (xTIM_TypeDef*)xTIM13_BASE,
        (xTIM_TypeDef*)xTIM14_BASE
};

/* ---------- 辅助：时钟使能 ---------- */
void hal_tim_clock_enable(tim_id_t id)
{
    // 根据 id 使用 RCC 驱动使能时钟
    // 此处使用直接寄存器写入，也可以调用 rcc_periph_clock_enable
    //volatile uint32_t *APB1ENR = (uint32_t*)0x40023840UL;
    //volatile uint32_t *APB2ENR = (uint32_t*)0x40023844UL;

    switch (id) {
        case TIM_1:
            //*APB2ENR |= (1 << 0);
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM1EN);
            break;
        case TIM_2:
            //*APB1ENR |= (1 << 0);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM2EN);
            break;
        case TIM_3:
            //*APB1ENR |= (1 << 1);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM3EN);
            break;
        case TIM_4:
            //*APB1ENR |= (1 << 2);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM4EN);
            break;
        case TIM_5:
            //*APB1ENR |= (1 << 3);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM5EN);
            break;
        case TIM_6:
            //*APB1ENR |= (1 << 4);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM6EN);
            break;
        case TIM_7:
            //*APB1ENR |= (1 << 5);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM7EN);
            break;
        case TIM_8:
           // *APB2ENR |= (1 << 1);
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM8EN);
            break;
        case TIM_9:
            //*APB2ENR |= (1 << 16);
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM9EN);
            break;
        case TIM_10:
           // *APB2ENR |= (1 << 17);
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM10EN);
            break;
        case TIM_11:
            //*APB2ENR |= (1 << 18);
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM11EN);
            break;
        case TIM_12:
           // *APB1ENR |= (1 << 6);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM12EN);
            break;
        case TIM_13:
           // *APB1ENR |= (1 << 7);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM13EN);
            break;
        case TIM_14:
            //*APB1ENR |= (1 << 8);
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM14EN);
            break;
        default:
            break;
    }
}
//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_timer.h"
#include "../common/rcc.h"
#include "../common/nvic.h"

/* ---------- TIM 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;          // 重复计数，高级定时器
    volatile uint32_t CCR[4];       // 通道比较/捕获寄存器
    volatile uint32_t BDTR;         // 断路和死区，高级定时器
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} xTIM_TypeDef;

static xTIM_TypeDef* const TIMx[TIM_MAX] = {
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
    switch (id) {
        case TIM_1:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM1EN);
            break;
        case TIM_2:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM2EN);
            break;
        case TIM_3:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM3EN);
            break;
        case TIM_4:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM4EN);
            break;
        case TIM_5:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM5EN);
            break;
        case TIM_6:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM6EN);
            break;
        case TIM_7:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM7EN);
            break;
        case TIM_8:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM8EN);
            break;
        case TIM_9:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM9EN);
            break;
        case TIM_10:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM10EN);
            break;
        case TIM_11:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_TIM11EN);
            break;
        case TIM_12:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM12EN);
            break;
        case TIM_13:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM13EN);
            break;
        case TIM_14:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_TIM14EN);
            break;
        default:
            break;
    }
}

nvic_irq_num hal_tim_get_irqn(tim_id_t id)
{
    switch (id) {
        case TIM_1:
            return TIM1_UP_TIM10_IRQ;   // 实际上是 TIM1_UP_TIM10? 需按手册确认
        case TIM_2:
            return TIM2_IRQ;
        case TIM_3:
            return TIM3_IRQ;
        case TIM_4:
            return TIM4_IRQ;
        case TIM_5:
            return TIM5_IRQ;
        case TIM_6:
            return TIM6_DAC_IRQ;
        case TIM_7:
            return TIM7_IRQ;
        case TIM_8:
            return TIM8_UP_TIM13_IRQ;
        case TIM_9:
            return TIM1_BRK_TIM9_IRQ;
        case TIM_10:
            return TIM1_UP_TIM10_IRQ;
        case TIM_11:
            return TIM1_TRG_COM_TIM11_IRQ;
        case TIM_12:
            return TIM8_BRK_TIM12_IRQ;
        case TIM_13:
            return TIM8_UP_TIM13_IRQ;
        case TIM_14:
            return TIM8_TRG_COM_TIM14_IRQ;
        default:
            return MAX_IRQ;
    }
}

void hal_tim_timebase_init(tim_id_t id, const tim_timebase_t *timebase) {
    xTIM_TypeDef *tim = TIMx[id];
    /* ---- 1. 时基配置 ---- */
    tim->PSC = timebase->prescaler;
    tim->ARR = timebase->autoreload;
    tim->RCR = timebase->repetition;   // 高级定时器有效，其他忽略
    uint32_t cr1 = 0;
    if (timebase->counter_mode == TIM_COUNTER_UP) {
        cr1 |= (0 << 4);
    } else if (timebase->counter_mode == TIM_COUNTER_DOWN) {
        cr1 |= (1 << 4);
    } else {    // 中央对齐
        cr1 |= (3 << 4);
    }
    cr1 |= (timebase->clock_division & 0x3) << 8;  // CKD
    tim->CR1 = cr1;
}

void hal_tim_oc_channel_init(tim_id_t id, uint8_t num_oc_channels, const tim_oc_channel_t *oc_channels) {
    xTIM_TypeDef *tim = TIMx[id];
    /* ---- 2. 输出比较通道 ---- */
    uint32_t ccer = tim->CCER;  // 保留之前值，其实应该清零
    uint32_t ccmr1 = tim->CCMR1;
    uint32_t ccmr2 = tim->CCMR2;
    for (int i = 0; i < num_oc_channels; i++) {
        const tim_oc_channel_t *ch = oc_channels + i;
        uint8_t idx = ch->channel - 1;
        if (idx > 3) {
            continue;
        }
        tim->CCR[idx] = ch->pulse;

        // 配置输出模式
        uint8_t mode = ch->mode & 0x7;
        if (idx < 2) {
            // CCMR1
            uint32_t shift = 8 * idx;
            ccmr1 &= ~(0x7 << (shift + 4));
            ccmr1 |= (mode << (shift + 4));          // OCxM
            if (ch->enable_preload) {
                ccmr1 |= (1 << (shift + 3));         // OCxPE
            } else {
                ccmr1 &= ~(1 << (shift + 3));
            }
        } else {
            // CCMR2 (channel 3/4)
            uint32_t shift = 8 * (idx - 2);
            ccmr2 &= ~(0x7 << (shift + 4));
            ccmr2 |= (mode << (shift + 4));
            if (ch->enable_preload) {
                ccmr2 |= (1 << (shift + 3));
            } else {
                ccmr2 &= ~(1 << (shift + 3));
            }
        }

        // 使能输出 (CCER)
        ccer |= (1 << (idx * 4));   // CCxE
        //tim_oc_enabled[timer->conf->id] |= (1 << idx);
    }
    tim->CCMR1 = ccmr1;
    tim->CCMR2 = ccmr2;
    tim->CCER = ccer;
}

void hal_tim_ic_channel_init(tim_id_t id, uint8_t num_ic_channels, const tim_ic_channel_t *ic_channels) {
    xTIM_TypeDef *tim = TIMx[id];
    /* ---- 3. 输入捕获通道 ---- */
    uint32_t ccer = tim->CCER;  // 保留之前值，其实应该清零
    uint32_t ccmr1 = tim->CCMR1;  // 重新读取
    uint32_t ccmr2 = tim->CCMR2;
    for (int i = 0; i < num_ic_channels; i++) {
        const tim_ic_channel_t *ch = ic_channels + i;
        uint8_t idx = ch->channel - 1;
        if (idx > 3) {
            continue;
        }

        // 输入配置
        uint8_t ic_config = 0;
        switch (ch->edge) {
            case TIM_IC_EDGE_RISING:
                ic_config = 1;
                break;
            case TIM_IC_EDGE_FALLING:
                ic_config = 2;
                break;
            case TIM_IC_EDGE_BOTH:
                ic_config = 3;
                break;
        }
        if (idx < 2) {
            uint32_t shift = 8 * idx;
            ccmr1 &= ~(0x3 << shift);
            ccmr1 |= (ic_config << shift);           // CCxS
            ccmr1 &= ~(0xF << (shift + 4));
            ccmr1 |= ((ch->prescaler & 3) << (shift + 2)) | ((ch->filter & 0xF) << (shift + 4));
        } else {
            uint32_t shift = 8 * (idx - 2);
            ccmr2 &= ~(0x3 << shift);
            ccmr2 |= (ic_config << shift);
            ccmr2 &= ~(0xF << (shift + 4));
            ccmr2 |= ((ch->prescaler & 3) << (shift + 2)) | ((ch->filter & 0xF) << (shift + 4));
        }
        // 输入捕获不需要使能CCER输出，但需要使能输入捕获
        ccer |= (1 << (idx * 4 + 1));  // 软件捕获标志？实际上输入捕获时 CCxE 可以不用，但 CCxP 极性通过 edge 配置？
        // 极性配置：根据 edge 写入 CCER 的 CCxP 和 CCxNP
        // 简化：这里暂不处理详细极性，可后续扩展
    }
    tim->CCMR1 = ccmr1;
    tim->CCMR2 = ccmr2;
    tim->CCER = ccer;
}

void hal_tim_disable_it(tim_id_t id) {
    xTIM_TypeDef *tim = TIMx[id];
    tim->DIER = 0;
}

bool hal_tim_it_init(tim_id_t id, tim_it_t it_enable) {
    xTIM_TypeDef *tim = TIMx[id];
    if (it_enable) {
        //tim_callbacks[timer->conf->id] = timer->conf->callback;
        uint32_t dier = 0;
        if (it_enable & TIM_IT_UPDATE) {
            dier |= (1 << 0);
        }
        if (it_enable & TIM_IT_CC1) {
            dier |= (1 << 1);
        }
        if (it_enable & TIM_IT_CC2) {
            dier |= (1 << 2);
        }
        if (it_enable & TIM_IT_CC3) {
            dier |= (1 << 3);
        }
        if (it_enable & TIM_IT_CC4) {
            dier |= (1 << 4);
        }
        tim->DIER = dier;
        return true;
    }
    return false;
}

void hal_tim_start(tim_id_t id) {
    if (id < TIM_MAX) {
        TIMx[id]->CR1 |= (1 << 0);   // CEN
    }
}

void hal_tim_stop(tim_id_t id) {
    if (id < TIM_MAX) {
        TIMx[id]->CR1 &= ~(1 << 0);
    }
}

void hal_tim_set_pulse(tim_id_t id, uint8_t channel, uint32_t pulse) {
    if (id >= TIM_MAX || channel < 1 || channel > 4) {
        return ;
    }
    TIMx[id]->CCR[channel-1] = pulse;
}

uint32_t hal_tim_get_capture(tim_id_t id, uint8_t channel) {
    if (id >= TIM_MAX || channel < 1 || channel > 4) {
        return 0;
    }
    return TIMx[id]->CCR[channel-1];
}

uint32_t hal_tim_get_counter(tim_id_t id) {
    if (id >= TIM_MAX) {
        return 0;
    }
    return TIMx[id]->CNT;
}

void hal_tim_set_period(tim_id_t id, uint32_t autoreload) {
    if (id >= TIM_MAX) {
        return;
    }
    // 直接写 ARR 寄存器 (需要定时器处于运行状态)
    TIMx[id]->ARR = autoreload;
}

uint32_t hal_tim_get_it_event(tim_id_t id) {
    xTIM_TypeDef *tim = TIMx[id];
    uint32_t sr = tim->SR;
    return sr;
}

void hal_tim_clear_it_event(tim_id_t id, uint8_t it_event) {
    xTIM_TypeDef *tim = TIMx[id];
    tim->SR &= ~(it_event);
}
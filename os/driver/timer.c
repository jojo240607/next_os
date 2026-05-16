#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_timer.h"


static void timer_start(Timer* self);
static void timer_stop(Timer* self);
static void timer_set_pulse(Timer* self, uint8_t channel, uint32_t pulse);
static uint32_t timer_get_capture(Timer* self, uint8_t channel);
static uint32_t timer_get_counter(Timer* self);

dev_init_override(timer_dev_init_impl);

// 析构函数声明
static void timer_destroy(Timer* self);

// TODO: 初始化数据成员
static const TimerFun timer_fun = {
    .destroy = timer_destroy,
	.start = timer_start,
	.stop = timer_stop,
	.set_pulse = timer_set_pulse,
	.get_capture = timer_get_capture,
	.get_counter = timer_get_counter,
};

// 构造函数实现
Timer* timer_create(const tim_config_t *conf) {
    Timer* obj = (Timer*)os_malloc(sizeof(Timer));
    if (obj) {
        memset(obj, 0, sizeof(Timer));
        timer_init(obj, conf);
    }
    return obj;
}

void timer_init(Timer* self, const tim_config_t *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(timer_fun);
    // TODO: 初始化派生类特有成员
    self->conf = conf;
	def_dev_init(self) = timer_dev_init_impl;
}


static nvic_irq_num tim_get_irqn(tim_id_t id)
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

void timer_deinit(Timer* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void timer_destroy(Timer* self) {
    if (self != NULL) {
        timer_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(timer_dev_init_impl) {
    // TODO: add dev_init method
    Timer *timer = (Timer *)self;
    LOG_DEBUG("timer", "timer init");
    //params
    if (!timer->conf || timer->conf->id >= TIM_MAX) {
        return;
    }

    xTIM_TypeDef *tim = TIMx[timer->conf->id];
    hal_tim_clock_enable(timer->conf->id);

    /* ---- 1. 时基配置 ---- */
    tim->PSC = timer->conf->timebase.prescaler;
    tim->ARR = timer->conf->timebase.autoreload;
    tim->RCR = timer->conf->timebase.repetition;   // 高级定时器有效，其他忽略
    uint32_t cr1 = 0;
    if (timer->conf->timebase.counter_mode == TIM_COUNTER_UP) {
        cr1 |= (0 << 4);
    } else if (timer->conf->timebase.counter_mode == TIM_COUNTER_DOWN) {
        cr1 |= (1 << 4);
    } else {    // 中央对齐
        cr1 |= (3 << 4);
    }
    cr1 |= (timer->conf->timebase.clock_division & 0x3) << 8;  // CKD
    tim->CR1 = cr1;

    /* ---- 2. 输出比较通道 ---- */
    uint32_t ccer = tim->CCER;  // 保留之前值，其实应该清零
    uint32_t ccmr1 = tim->CCMR1;
    uint32_t ccmr2 = tim->CCMR2;
    for (int i = 0; i < timer->conf->num_oc_channels; i++) {
        const tim_oc_channel_t *ch = &timer->conf->oc_channels[i];
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

    /* ---- 3. 输入捕获通道 ---- */
    ccmr1 = tim->CCMR1;  // 重新读取
    ccmr2 = tim->CCMR2;
    for (int i = 0; i < timer->conf->num_ic_channels; i++) {
        const tim_ic_channel_t *ch = &timer->conf->ic_channels[i];
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

    /* ---- 4. 中断配置 ---- */
    tim->DIER = 0;  // 先清
    if (timer->conf->it_enable) {
        //tim_callbacks[timer->conf->id] = timer->conf->callback;
        uint32_t dier = 0;
        if (timer->conf->it_enable & TIM_IT_UPDATE) {
            dier |= (1 << 0);
        }
        if (timer->conf->it_enable & TIM_IT_CC1) {
            dier |= (1 << 1);
        }
        if (timer->conf->it_enable & TIM_IT_CC2) {
            dier |= (1 << 2);
        }
        if (timer->conf->it_enable & TIM_IT_CC3) {
            dier |= (1 << 3);
        }
        if (timer->conf->it_enable & TIM_IT_CC4) {
            dier |= (1 << 4);
        }
        tim->DIER = dier;

       // nvic_enable_irq(tim_get_irqn(timer->conf->id));
        //nvic_set_priority(tim_get_irqn(timer->conf->id), 1, 0);

        self->irq_conf.priority = self->fun->encode_pripority(self, 0x01, 0x00);
        self->irq_conf.handler = timer_irq_handler_impl;
        self->irq_conf.semaphore = sem;
        self->irq_conf.arg = self;
        self->irq_conf.irq_num = tim_get_irqn(timer->conf->id);
        self->fun->attach_irq(self, &self->irq_conf);
    }

}

// irq_handler method
bool timer_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    Timer *timer = (Timer *)arg;
    //params , void *arg
    // 检查更新中断标志 (UIF)
    if (timer->conf->id >= TIM_MAX) {
        return true;
    }

    xTIM_TypeDef *tim = TIMx[timer->conf->id];
    uint32_t sr = tim->SR;
    uint32_t event = 0xFF;  // 无效事件

    if (sr & (1 << 0)) {  // UIF
        event = TIM_EVT_UPDATE;
        tim->SR &= ~(1 << 0);
    }
    if (sr & (1 << 1)) {
        event = TIM_EVT_CC1;
        tim->SR &= ~(1 << 1);
    }
    if (sr & (1 << 2)) {
        event = TIM_EVT_CC2;
        tim->SR &= ~(1 << 2);
    }
    if (sr & (1 << 3)) {
        event = TIM_EVT_CC3;
        tim->SR &= ~(1 << 3);
    }
    if (sr & (1 << 4)) {
        event = TIM_EVT_CC4;
        tim->SR &= ~(1 << 4);
    }

    //if (event != 0xFF && tim_callbacks[id])
    //    tim_callbacks[id](id, event);
    return true;
}
// start method
static void timer_start(Timer* self) {
    if (self->conf->id < TIM_MAX) {
        TIMx[self->conf->id]->CR1 |= (1 << 0);   // CEN
    }
}
// stop method
static void timer_stop(Timer* self) {
    if (self->conf->id < TIM_MAX) {
        TIMx[self->conf->id]->CR1 &= ~(1 << 0);
    }
}
// set_pulse method
static void timer_set_pulse(Timer* self, uint8_t channel, uint32_t pulse) {
    if (self->conf->id >= TIM_MAX || channel < 1 || channel > 4) {
        return ;
    }
    TIMx[self->conf->id]->CCR[channel-1] = pulse;
}
// get_capture method
static uint32_t timer_get_capture(Timer* self, uint8_t channel) {
    if (self->conf->id >= TIM_MAX || channel < 1 || channel > 4) {
        return 0;
    }
    return TIMx[self->conf->id]->CCR[channel-1];
}
// get_counter method
static uint32_t timer_get_counter(Timer* self) {
    if (self->conf->id >= TIM_MAX) {
        return 0;
    }
    return TIMx[self->conf->id]->CNT;
}


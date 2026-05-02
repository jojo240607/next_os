#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"

dev_init_override(timer_dev_init_impl);

// 析构函数声明
static void timer_destroy(Timer* self);

// TODO: 初始化数据成员
static const TimerFun timer_fun = {
    .destroy = timer_destroy,
};
static TIM_TypeDef* const TIMx[] = {
        (TIM_TypeDef*)TIM1,
        (TIM_TypeDef*) TIM2,
        (TIM_TypeDef*)TIM3,
        (TIM_TypeDef*)TIM4,
        (TIM_TypeDef*)TIM5,
        (TIM_TypeDef*)TIM6,
        (TIM_TypeDef*)TIM7,
        (TIM_TypeDef*)TIM8,
        (TIM_TypeDef*)TIM9,
        (TIM_TypeDef*)TIM10,
        (TIM_TypeDef*)TIM11,
        (TIM_TypeDef*)TIM12,
        (TIM_TypeDef*)TIM13,
        (TIM_TypeDef*)TIM14,
};
// 构造函数实现
Timer* timer_create(timer_config *conf) {
    Timer* obj = (Timer*)os_malloc(sizeof(Timer));
    if (obj) {
        memset(obj, 0, sizeof(Timer));
        timer_init(obj, conf);
    }
    return obj;
}

void timer_init(Timer* self, timer_config *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(timer_fun);
    // TODO: 初始化派生类特有成员
    self->conf = conf;
	def_dev_init(self) = timer_dev_init_impl;
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
    //params
    switch (timer->conf->timer_id) {
        case TIME_1:
            RCC->APB1ENR |= RCC_APB2ENR_TIM1EN;
            self->irq_conf.irq_num = TIM1_CC_IRQ;
            break;
        case TIME_2:
            // 1. 使能 TIM2 时钟 (RCC APB1 外设时钟使能寄存器)
            RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
            self->irq_conf.irq_num = TIM2_IRQ;
            break;
        case TIME_3:
            RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
            self->irq_conf.irq_num = TIM3_IRQ;
        case TIME_4:
            RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
            self->irq_conf.irq_num = TIM4_IRQ;
        case TIME_5:
            RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
            self->irq_conf.irq_num = TIM5_IRQ;
        case TIME_6:
            RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
            self->irq_conf.irq_num = TIM6_DAC_IRQ;
            break;
        case TIME_7:
            RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
            self->irq_conf.irq_num = TIM7_IRQ;
            break;
        case TIME_8:
            RCC->APB1ENR |= RCC_APB2ENR_TIM8EN;
            self->irq_conf.irq_num = TIM8_CC_IRQ;
            break;
        case TIME_9:
            RCC->APB1ENR |= RCC_APB2ENR_TIM9EN;
            self->irq_conf.irq_num = TIM1_BRK_TIM9_IRQ;
        case TIME_10:
            RCC->APB1ENR |= RCC_APB2ENR_TIM10EN;
            self->irq_conf.irq_num = TIM1_UP_TIM10_IRQ;
        case TIME_11:
            RCC->APB1ENR |= RCC_APB2ENR_TIM11EN;
            self->irq_conf.irq_num = TIM1_TRG_COM_TIM11_IRQ;
        case TIME_12:
            RCC->APB1ENR |= RCC_APB1ENR_TIM12EN;
            self->irq_conf.irq_num = TIM8_BRK_TIM12_IRQ;
            break;
        case TIME_13:
            RCC->APB1ENR |= RCC_APB1ENR_TIM13EN;
            self->irq_conf.irq_num = TIM8_UP_TIM13_IRQ;
            break;
        case TIME_14:
            RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
            self->irq_conf.irq_num = TIM8_TRG_COM_TIM14_IRQ;
            break;
        default:
            break;
    }

    self->irq_conf.priority = NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
    self->irq_conf.handler = timer_irq_handler_impl;
    self->irq_conf.semaphore = sem;
    self->irq_conf.arg = self;

    // 2. 设置预分频器 (PSC) 和 自动重装载 (ARR)
    //PSC + 1 = 16800     PSC+1=16800 ⇒ 计数时钟 = 168 MHz / 16800 = 10 kHz，周期 100 μs
    //同样需要 5000 个计数 ⇒  ARR = 4999 ARR=4999
    //uint16_t psc = (uint32_t)(tim_clock_hz / desired_cnt_Hz) - 1;
    //uint32_t arr = (uint32_t)(desired_cnt_Hz * target_seconds) - 1;
    TIMx[timer->conf->timer_id]->PSC = 16799;      // 分频系数 16800
    TIMx[timer->conf->timer_id]->ARR = 499;       // 自动重装载值，产生 500ms 周期

    // 3. 生成更新事件，让新值立即生效 (可选)
    TIMx[timer->conf->timer_id]->EGR |= TIM_EGR_UG;

    if (self->irq_conf.handler != NULL) {
        // 4. 使能更新中断 (UIE)
        TIMx[timer->conf->timer_id]->DIER |= TIM_DIER_UIE;
        if (!self->fun->attach_irq(self, &self->irq_conf)) {
            LOG_ERROR("timer", "attach irq %d error", self->irq_conf.irq_num);
        }
    }

    // 6. 启动定时器 (CEN 位)
    TIMx[timer->conf->timer_id]->CR1 |= TIM_CR1_CEN;
}

// irq_handler method
bool timer_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    Timer *timer = (Timer *)arg;
    //params , void *arg
    // 检查更新中断标志 (UIF)
    if (TIMx[timer->conf->timer_id]->SR & TIM_SR_UIF) {
        // 清除中断标志 (写 0 无效，需要写 0 的方式是清对应位)
        TIMx[timer->conf->timer_id]->SR &= ~TIM_SR_UIF;
        // 这里放置你需要每 500ms 执行的代码
        // 例如：翻转 LED，读取传感器等
       // LOG_DEBUG("timer irq", "----- time in -----");
    }
    return true;
}
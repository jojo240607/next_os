#include "exti.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "../task/os_cb_task.h"
#include "hal/hal_exti.h"

dev_init_override(exti_dev_init_impl);

// 析构函数声明
static void exti_destroy(Exti* self);

// TODO: 初始化数据成员
static const ExtiFun exti_fun = {
    .destroy = exti_destroy,

};
// 构造函数实现
Exti* exti_create(const exti_config *conf, const dev_pripority_t *priority) {
    Exti* obj = (Exti*)os_malloc(sizeof(Exti));
    if (obj) {
        memset(obj, 0, sizeof(Exti));
        exti_init(obj, conf, priority);
    }
    return obj;
}


void exti_init(Exti* self, const exti_config *conf, const dev_pripority_t *priority) {
    // 初始化基类部分
    device_init(&self->base, priority);
    self->fun = &(exti_fun);
    // TODO: 初始化派生类特有成员
	def_dev_init(self) = exti_dev_init_impl;
    self->conf = conf;

}

void exti_deinit(Exti* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放

}
// 析构函数实现
static void exti_destroy(Exti* self) {
    if (self != NULL) {
        exti_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(exti_dev_init_impl) {
    // TODO: add dev_init method
    LOG_DEBUG("exti", "exti init");
    Exti *exti = (Exti *)self;
    //params
    hal_syscfg_clock_enable();
    /* 1. 通过 PinMux 将所有引脚设为模拟模式 */
    for (int i = 0; i < exti->conf->pin_size; i++) {
        pin_config_t pin_cfg = {
                .port   = exti->conf->pin_conf[i]->port,
                .pin    = exti->conf->pin_conf[i]->pin,
                .mode = PIN_MODE_INPUT,
                .otype = PIN_OTYPE_OD,
                .ospeed = PIN_OSPEED_HIGH,
                .pupd = PIN_PUPD_PULLUP,
                .irq_mode = exti->conf->pin_conf[i]->exti_mode,

        };
        if (pinmux_request(&pin_cfg) != PINMUX_SUCCESS) {
            LOG_ERROR("exti", "pinmux error");
            return;
        }
    }
    self->irq_conf.handler = exti_irq_handler_impl;
    self->irq_conf.semaphore = sem;
    self->irq_conf.arg = self;

    for (uint8_t num = 0; num < exti->conf->pin_size; num++) {
        nvic_irq_num new_irq_num;
        if (exti->conf->pin_conf[num]->pin < 5) {
            new_irq_num = EXTI0_IRQ + exti->conf->pin_conf[num]->pin;
        } else if (exti->conf->pin_conf[num]->pin < 10) {
            new_irq_num = EXTI5_9_IRQ;
        } else {
            new_irq_num = EXTI10_15_IRQ;
        }
        self->irq_conf.irq_num = new_irq_num;
        if (self->irq_conf.handler != NULL) {
            if (!self->fun->attach_irq(self, &self->irq_conf)) {
                LOG_ERROR("exti", "attach irq %d error", self->irq_conf.irq_num);
            }
        }
    }
}

// irq_handler method
bool exti_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    Exti *exti = (Exti *)arg;
    //params , void *arg

    // 检查是否是EXTI线0的中断挂起标志
    while (xEXTI->PR) {
        // 最低位 1 的索引（0~15）
        uint32_t bit = __builtin_ctz(xEXTI->PR);
        //printf("Need to process bit %d\n", bit);
        GET_DEVICE(exti)->irq_conf.semaphore->sem_event = OS_EVENT_EXTI0 + bit;
        //  清除中断标志位（写1清零）
        xEXTI->PR |= (1 << bit);
    }
    return true;
}



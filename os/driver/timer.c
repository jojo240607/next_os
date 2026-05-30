#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_timer.h"


dev_ioctl_override(timer_dev_ioctl_impl);

static void timer_set_period(Timer* self, uint32_t autoreload);

static void timer_start(Timer* self);
static void timer_stop(Timer* self);
static void timer_set_pulse(Timer* self, uint8_t channel, uint32_t pulse);
static uint32_t timer_get_capture(Timer* self, uint8_t channel);
static uint32_t timer_get_counter(Timer* self);

dev_init_override(timer_dev_init_impl);

// 析构函数声明
static void timer_destroy(Timer* self);
static bool timer_irq_handler_impl(nvic_irq_t *irq_conf);
// TODO: 初始化数据成员
static const TimerFun timer_fun = {
    .destroy = timer_destroy,
	.set_pulse = timer_set_pulse,
	.get_capture = timer_get_capture,
	.get_counter = timer_get_counter,
	.set_period = timer_set_period,
};

// 构造函数实现
Timer* timer_create(const device_info_t *info) {
    Timer* obj = (Timer*)os_malloc(sizeof(Timer));
    if (obj) {
        memset(obj, 0, sizeof(Timer));
        timer_init(obj, info);
    }
    return obj;
}

void timer_init(Timer* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(timer_fun);
    // TODO: 初始化派生类特有成员
	def_dev_init(self) = timer_dev_init_impl;
	def_dev_ioctl(self) = timer_dev_ioctl_impl;
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
    const tim_config_t *conf = self->info->conf;
    LOG_DEBUG("timer", "timer init");
    //params
    if (!conf || conf->id >= TIM_MAX) {
        return;
    }
    hal_tim_clock_enable(conf->id);
    hal_tim_timebase_init(conf->id, &conf->timebase);
    hal_tim_oc_channel_init(conf->id, conf->num_oc_channels, conf->oc_channels);
    hal_tim_ic_channel_init(conf->id, conf->num_ic_channels, conf->ic_channels);

    /* ---- 4. 中断配置 ---- */
    hal_tim_disable_it(conf->id);  // 先清
    if (hal_tim_it_init(conf->id, conf->it_enable)) {
        self->irq_conf->handler = timer_irq_handler_impl;
        self->irq_conf->arg = self;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, hal_tim_get_irqn(conf->id));
        self->fun->config_irq(self, self->irq_conf);
    }

}

// irq_handler method
static bool timer_irq_handler_impl(nvic_irq_t *irq_conf) {
    // TODO: add irq_handler method
    Timer *timer = (Timer *)irq_conf->arg;
    const tim_config_t *conf = GET_DEVICE(timer)->info->conf;
    //params , void *arg
    // 检查更新中断标志 (UIF)
    if (conf->id >= TIM_MAX) {
        return true;
    }

    uint32_t sr = hal_tim_get_it_event(conf->id);
    //uint32_t event = 0xFF;  // 无效事件
    if (sr & TIM_IT_UPDATE) {  // UIF
        //event = TIM_EVT_UPDATE;
        hal_tim_clear_it_event(conf->id, TIM_IT_UPDATE);
    }
    if (sr & TIM_IT_CC1) {
        //event = TIM_EVT_CC1;
        hal_tim_clear_it_event(conf->id, TIM_IT_CC1);
    }
    if (sr & TIM_IT_CC2) {
        //event = TIM_EVT_CC2;
        hal_tim_clear_it_event(conf->id, TIM_IT_CC2);
    }
    if (sr & TIM_IT_CC3) {
        //event = TIM_EVT_CC3;
        hal_tim_clear_it_event(conf->id, TIM_IT_CC3);
    }
    if (sr & TIM_IT_CC4) {
        //event = TIM_EVT_CC4;
        hal_tim_clear_it_event(conf->id, TIM_IT_CC4);
    }

    //if (event != 0xFF && tim_callbacks[id])
    //    tim_callbacks[id](id, event);
    return true;
}
// start method
static void timer_start(Timer* self) {
    const tim_config_t *conf = GET_DEVICE(self)->info->conf;
    hal_tim_start(conf->id);
}
// stop method
static void timer_stop(Timer* self) {
    const tim_config_t *conf = GET_DEVICE(self)->info->conf;
    hal_tim_stop(conf->id);
}
// set_pulse method
static void timer_set_pulse(Timer* self, uint8_t channel, uint32_t pulse) {
    const tim_config_t *conf = GET_DEVICE(self)->info->conf;
    hal_tim_set_pulse(conf->id, channel, pulse);
}
// get_capture method
static uint32_t timer_get_capture(Timer* self, uint8_t channel) {
    const tim_config_t *conf = GET_DEVICE(self)->info->conf;
    return hal_tim_get_capture(conf->id, channel);
}
// get_counter method
static uint32_t timer_get_counter(Timer* self) {
    const tim_config_t *conf = GET_DEVICE(self)->info->conf;
    return hal_tim_get_counter(conf->id);
}

// set_period method
static void timer_set_period(Timer* self, uint32_t autoreload) {
    const tim_config_t *conf = GET_DEVICE(self)->info->conf;
    hal_tim_set_period(conf->id, autoreload);
}


// dev_ioctl method
dev_ioctl_override(timer_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Timer *timer = (Timer *)self;
    //params , ioctl_cmd_t cmd, void *arg
    if (cmd == DEVICE_START) {
        const tim_config_t *conf = GET_DEVICE(self)->info->conf;
        hal_tim_start(conf->id);
    } else if (cmd == DEVICE_STOP) {
        const tim_config_t *conf = GET_DEVICE(self)->info->conf;
        hal_tim_stop(conf->id);
    }
}


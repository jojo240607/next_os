#include "pwm.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"


dev_ioctl_override(pwm_dev_ioctl_impl);

static void pwm_set_duty(Pwm* self, uint8_t channel, uint32_t duty);
static void pwm_set_period(Pwm* self, uint32_t autoreload);

dev_init_override(pwm_dev_init_impl);

// 析构函数声明
static void pwm_destroy(Pwm* self);

// TODO: 初始化数据成员
static const PwmFun pwm_fun = {
    .destroy = pwm_destroy,
	.set_duty = pwm_set_duty,
	.set_period = pwm_set_period,
};
// 构造函数实现
Pwm* pwm_create(const device_info_t *info) {
    Pwm* obj = (Pwm*)os_malloc(sizeof(Pwm));
    if (obj) {
        memset(obj, 0, sizeof(Pwm));
        pwm_init(obj, info);
    }
    return obj;
}

void pwm_init(Pwm* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(pwm_fun);
    // TODO: 初始化派生类特有成员
	def_dev_init(self) = pwm_dev_init_impl;
	def_dev_ioctl(self) = pwm_dev_ioctl_impl;
}

void pwm_deinit(Pwm* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void pwm_destroy(Pwm* self) {
    if (self != NULL) {
        pwm_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(pwm_dev_init_impl) {
    // TODO: add dev_init method
    Pwm *pwm = (Pwm *)self;
    const pwm_config_t *conf = self->info->conf;
    LOG_DEBUG("pwm", "pwm init!");
    //params , int cmd, void *arg
    if (!conf || conf->timer_id >= TIM_MAX) {
        return;
    }
    // 1. 配置所有 PWM 引脚 (复用推挽输出)
    for (int i = 0; i < conf->num_channels; i++) {
        const pwm_channel_t *ch = conf->channels[i];
        pin_config_t pin_cfg = {
                .mode   = PIN_MODE_AF,
                .otype  = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_HIGH,
                .pupd   = PIN_PUPD_NONE,
                .af     = ch->pwm_pin,
        };
        if (pinmux_request(&pin_cfg) != PINMUX_SUCCESS) {
            LOG_DEBUG("pwm", "pinmux set error!");
            return;
        }
    }
    // 2. 构建底层 timer 配置
    tim_config_t tim_cfg = {
            .id             = conf->timer_id,
            .timebase       = conf->timebase,
            .it_enable      = TIM_IT_NONE,            // PWM 一般不需要定时器中断
            .num_oc_channels = conf->num_channels,
            .num_ic_channels = 0
    };

    // 3. 填充输出比较通道
    for (int i = 0; i < conf->num_channels; i++) {
        tim_cfg.oc_channels[i].channel        = conf->channels[i]->channel;
        tim_cfg.oc_channels[i].mode           = conf->channels[i]->mode;
        tim_cfg.oc_channels[i].pulse          = conf->channels[i]->duty;
        tim_cfg.oc_channels[i].enable_preload = conf->channels[i]->enable_preload;
    }

    const device_info_t *tim_info = &(const device_info_t){.name = self->info->name,
                                                           .id = self->info->id,
                                                           .priority = self->info->priority,
                                                           .conf = &tim_cfg};
    // 4. 调用底层定时器初始化
    pwm->time = timer_create(tim_info);
    if (pwm->time == NULL) {
        LOG_DEBUG("pwm", "timer create error");
        return;
    } else {
        virtual_dev_init(GET_DEVICE(pwm->time));
    }
}

// set_duty method
static void pwm_set_duty(Pwm* self, uint8_t channel, uint32_t duty) {
    // TODO: add set_duty method
    self->time->fun->set_pulse(self->time, channel, duty);
}
// set_period method
static void pwm_set_period(Pwm* self, uint32_t autoreload) {
    // TODO: add set_period method
    self->time->fun->set_period(self->time, autoreload);
}


// dev_ioctl method
dev_ioctl_override(pwm_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Pwm *pwm = (Pwm *)self;
    //params , ioctl_cmd_t cmd, void *arg
    if (cmd == DEVICE_START) {
        if (pwm->time != NULL) {
            GET_DEVICE(pwm->time)->vtable->dev_ioctl(GET_DEVICE(pwm->time), DEVICE_START, NULL);
        } else {
            LOG_ERROR("PWM", "time is null");
        }
    } else if (cmd == DEVICE_STOP) {
        if (pwm->time != NULL) {
            GET_DEVICE(pwm->time)->vtable->dev_ioctl(GET_DEVICE(pwm->time), DEVICE_STOP, NULL);
        } else {
            LOG_ERROR("PWM", "time is null");
        }
    }
}


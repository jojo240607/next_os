#include "systick.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../scheduler/thread_scheduler.h"
#include "../log/log.h"
#include "hal/hal_systick.h"
#include "common/rcc.h"
#include "hal/hal_svc.h"


dev_ioctl_override(systick_dev_ioctl_impl);

static void systick_stop(Systick* self);

static void systick_start(Systick* self);

dev_init_override(systick_dev_init_impl);

// 析构函数声明
static void systick_destroy(Systick* self);

// TODO: 初始化数据成员
static const SystickFun systick_fun = {
    .destroy = systick_destroy,
};
// 构造函数实现
Systick* systick_create(const device_info_t *info) {
    Systick* obj = (Systick*)os_malloc(sizeof(Systick));
    if (obj) {
        memset(obj, 0, sizeof(Systick));
        systick_init(obj, info);
    }
    return obj;
}

void systick_init(Systick* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(systick_fun);
    // TODO: 初始化派生类特有成员
	GET_DEVICE_VTABLE(self)->dev_init = systick_dev_init_impl;
	def_dev_ioctl(self) = systick_dev_ioctl_impl;
}

void systick_deinit(Systick* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void systick_destroy(Systick* self) {
    if (self != NULL) {
        systick_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(systick_dev_init_impl) {
    Systick *systick = (Systick *)self;
    LOG_DEBUG("systick", "systick init");
    const systick_config_t *conf = self->info->conf;
    // TODO: add dev_init method
    if (!conf || conf->interval_us == 0) {
        return;
    }
    hal_systick_init(conf->interval_us);

    self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, SYSTIC_IRQ);
    self->irq_conf->handler = systick_irq_handler_impl;
    self->irq_conf->arg = self;
    if (!self->fun->config_irq(self, self->irq_conf)) {
        LOG_ERROR("systick", "attach SYSTIC_IRQ error");
    }
    //gloable_sys_time->time_us = hal_dwt_get_timestamp_us()->time_us;
}


// irq_handler method
bool systick_irq_handler_impl(nvic_irq_t *irq_conf) {
    // TODO: add irq_handler method
    //Systick *systick = (Systick *)arg;
    //params , void *arg
#ifdef USE_CCMRAM
    gloable_sys_time->time_us = hal_dwt_get_timestamp_us()->time_us;
#else
    gloable_sys_time->time_us ++;
#endif
    if (gloable_current_tcb->sp != NULL) {
        global_thread_scheduler->fun->delay_ticks(global_thread_scheduler);
        // 触发 PendSV 中断
        Trigger_PendSV;
    }
    return true;
}


// start method
static void systick_start(Systick* self) {
    hal_systick_start();
}


// stop method
static void systick_stop(Systick* self) {
    hal_systick_stop();
}


// dev_ioctl method
dev_ioctl_override(systick_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Systick *systick = (Systick *)self;
    //params , ioctl_cmd_t cmd, void *arg
    if (cmd == DEVICE_START) {
        hal_systick_start();
    } else if (cmd == DEVICE_STOP) {
        hal_systick_stop();
    }
}


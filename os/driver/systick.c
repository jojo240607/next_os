#include "systick.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../scheduler/thread_scheduler.h"
#include "../log/log.h"
#include "hal/hal_systick.h"
#include "common/rcc.h"


static void systick_stop(Systick* self);

static void systick_start(Systick* self);

dev_init_override(systick_dev_init_impl);

// 析构函数声明
static void systick_destroy(Systick* self);

// TODO: 初始化数据成员
static const SystickFun systick_fun = {
    .destroy = systick_destroy,
	.start = systick_start,
	.stop = systick_stop,
};
// 构造函数实现
Systick* systick_create(const systick_config_t *conf, const dev_pripority_t *priority) {
    Systick* obj = (Systick*)os_malloc(sizeof(Systick));
    if (obj) {
        memset(obj, 0, sizeof(Systick));
        systick_init(obj, conf, priority);
    }
    return obj;
}

void systick_init(Systick* self, const systick_config_t *conf, const dev_pripority_t *priority) {
    // 初始化基类部分
    device_init(&self->base, priority);
    self->fun = &(systick_fun);
    // TODO: 初始化派生类特有成员
	GET_DEVICE_VTABLE(self)->dev_init = systick_dev_init_impl;
    self->conf = conf;
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
    // TODO: add dev_init method
    if (!systick->conf || systick->conf->interval_us == 0) {
        return;
    }
    hal_systick_init(systick->conf->interval_us);

    self->irq_conf.irq_num = SYSTIC_IRQ;
    self->irq_conf.handler = systick_irq_handler_impl;
    self->irq_conf.semaphore = sem;
    self->irq_conf.arg = self;
    if (!self->fun->attach_irq(self, &self->irq_conf)) {
        LOG_ERROR("systick", "attach SYSTIC_IRQ error");
    }
}


// irq_handler method
bool systick_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    //Systick *systick = (Systick *)arg;
    //params , void *arg
    getSystime()->systick++;
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


#include "systick.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../scheduler/thread_scheduler.h"
#include "../log/log.h"
#include "hal/hal_systick.h"


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
Systick* systick_create(const systick_config_t *conf) {
    Systick* obj = (Systick*)os_malloc(sizeof(Systick));
    if (obj) {
        memset(obj, 0, sizeof(Systick));
        systick_init(obj, conf);
    }
    return obj;
}

void systick_init(Systick* self, const systick_config_t *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(systick_fun);
    // TODO: 初始化派生类特有成员

	GET_DEVICE_VTABLE(self)->dev_init = systick_dev_init_impl;
    //GET_DEVICE(self)->irq_num = SYSTIC_IRQ;
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
    if (!systick->conf || systick->conf->frequency_hz == 0 || systick->conf->interval_us == 0) {
        return;
    }

    /* 计算重装载值: 每微秒时钟周期数 = frequency_hz / 1000000 */
    uint32_t ticks_per_us = systick->conf->frequency_hz / 1000000UL;
    uint32_t reload = systick->conf->interval_us * ticks_per_us;

    /* 限制为 24 位 */
    if (reload > SYSTICK_MAX_RELOAD) {
        reload = SYSTICK_MAX_RELOAD;
    }

    /* 关闭定时器以确保安全配置 */
    xSYSTICK->CTRL = 0;
    xSYSTICK->LOAD = reload;
    xSYSTICK->VAL  = 0;  // 清除当前值
    self->irq_conf.irq_num = SYSTIC_IRQ;
    self->irq_conf.priority = self->fun->encode_pripority(self, 0x03, 0x03);
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
//    HAL_IncTick();
    /* USER CODE BEGIN SysTick_IRQn 1 */
    if (gloable_current_stack != NULL) {
        global_thread_scheduler->fun->delay_ticks(global_thread_scheduler);
        // 触发 PendSV 中断
        Trigger_PendSV;
    }
    return false;
}


// start method
static void systick_start(Systick* self) {
    uint32_t ctrl = 0;
    /* 使用处理器时钟 (HCLK) */
    ctrl |= SYSTICK_CTRL_CLKSOURCE;   // 1: 内核时钟
    /* 使能中断*/
    ctrl |= SYSTICK_CTRL_TICKINT;
    /* 使能计数器 */
    ctrl |= SYSTICK_CTRL_ENABLE;
    xSYSTICK->CTRL = ctrl;
}


// stop method
static void systick_stop(Systick* self) {
    xSYSTICK->CTRL &= ~SYSTICK_CTRL_ENABLE;
}


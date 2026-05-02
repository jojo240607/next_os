#include "systick.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../common/util.h"
#include "../scheduler/thread_scheduler.h"
#include "../log/log.h"


dev_init_override(systick_dev_init_impl);

// 析构函数声明
static void systick_destroy(Systick* self);

// TODO: 初始化数据成员
static const SystickFun systick_fun = {
    .destroy = systick_destroy,
};
// 构造函数实现
Systick* systick_create(systick_conf *conf) {
    Systick* obj = (Systick*)os_malloc(sizeof(Systick));
    if (obj) {
        memset(obj, 0, sizeof(Systick));
        systick_init(obj, conf);
    }
    return obj;
}

void systick_init(Systick* self, systick_conf *conf) {
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
    // TODO: add dev_init method
    self->irq_conf.irq_num = SYSTIC_IRQ;
    self->irq_conf.priority = NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x03, 0x03);
    self->irq_conf.handler = systick_irq_handler_impl;
    self->irq_conf.semaphore = sem;
    self->irq_conf.arg = self;
    if (!self->fun->attach_irq(self, &self->irq_conf)) {
        LOG_ERROR("systick", "attach SYSTIC_IRQ error");
    }
    //params 
    // 1. 设置重装载值，产生1ms中断
    //    (168000000 Hz / 1000) - 1 = 167999
    SysTick->LOAD = (systick->conf->systick_frequency / 1000) - 1;
    // 2. 设置优先级 (可选，在NVIC中设置)
    //    SysTick是内核中断，优先级通过SCB的SHPR3寄存器设置[reference:7]
    //    NVIC_SetPriority(SysTick_IRQn, 0x0F); // 使用CMSIS库函数

    // 3. 配置控制寄存器(CTRL)
    //    选择时钟源HCLK | 使能中断 | 使能定时器
    SysTick->CTRL = (1 << 2) |  // CLKSOURCE: HCLK (168MHz)
                    (1 << 1) |  // TICKINT: 使能中断
                    (1 << 0);   // ENABLE: 使能定时器

    // 4. 清空当前值寄存器，确保从LOAD值开始计数
    SysTick->VAL = 0;
}


// irq_handler method
bool systick_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    //Systick *systick = (Systick *)arg;
    //params , void *arg
    getSystime()->systick++;
    HAL_IncTick();
    /* USER CODE BEGIN SysTick_IRQn 1 */
    if (gloable_current_stack != NULL) {
        global_thread_scheduler->fun->delay_ticks(global_thread_scheduler);
        // 触发 PendSV 中断
        Trigger_PendSV;
    }
    return false;
}


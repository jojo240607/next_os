#include "systick.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../common/util.h"
#include "../scheduler/thread_scheduler.h"

irq_handler_override(systick_irq_handler_impl);

dev_init_override(systick_dev_init_impl);
dev_read_override(systick_dev_read_impl);
dev_write_override(systick_dev_write_impl);
dev_ioctl_override(systick_dev_ioctl_impl);

// 析构函数声明
static void systick_destroy(Systick* self);

// TODO: 初始化数据成员
static const SystickFun systick_fun = {
    .destroy = systick_destroy,
};
// 构造函数实现
Systick* systick_create() {
    Systick* obj = (Systick*)os_malloc(sizeof(Systick));
    if (obj) {
        memset(obj, 0, sizeof(Systick));
        systick_init(obj);
    }
    return obj;
}

void systick_init(Systick* self) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(systick_fun);
    // TODO: 初始化派生类特有成员

	GET_DEVICE_VTABLE(self)->dev_init = systick_dev_init_impl;
	GET_DEVICE_VTABLE(self)->dev_read = systick_dev_read_impl;
	GET_DEVICE_VTABLE(self)->dev_write = systick_dev_write_impl;
	GET_DEVICE_VTABLE(self)->dev_ioctl = systick_dev_ioctl_impl;
	def_irq_handler(self) = systick_irq_handler_impl;

    self->timetick = 0;
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
    // TODO: add dev_init method
    gloable_intc->fun->register_handler(gloable_intc, SYSTIC_IRQ, GET_DEVICE_VTABLE(self)->irq_handler, self);
    gloable_intc->fun->attach_semaphore(gloable_intc, SYSTIC_IRQ, sem);
    //params 
    
}
// dev_read method
dev_read_override(systick_dev_read_impl) {
    // TODO: add dev_read method
    Systick *systick = (Systick *)self;
    //params , void *buf, size_t count
    
}
// dev_write method
dev_write_override(systick_dev_write_impl) {
    // TODO: add dev_write method
    Systick *systick = (Systick *)self;
    //params , const void *buf, size_t count
    
}
// dev_ioctl method
dev_ioctl_override(systick_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Systick *systick = (Systick *)self;
    //params , int cmd, void *arg
    
}


// irq_handler method
irq_handler_override(systick_irq_handler_impl) {
    // TODO: add irq_handler method
    Systick *systick = (Systick *)arg;
    //params , void *arg
    systick->timetick++;
    HAL_IncTick();
    /* USER CODE BEGIN SysTick_IRQn 1 */
    if (gloable_current_stack != NULL) {
        global_thread_scheduler->fun->delay_ticks(global_thread_scheduler);
        // 触发 PendSV 中断
        Trigger_PendSV;
    }
    return false;
}


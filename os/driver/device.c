#include "device.h"
#include "../common/linear_pool.h"
#include <stdio.h>

static bool device_attach_irq(Device* self, irq_config *conf, Semaphore * sem);

static bool device_transfer(Device* self, const void *data, size_t count);

// 析构函数声明
static void device_destroy(Device* self);

// TODO: 初始化数据成员
static const DeviceFun device_fun = {
    .destroy = device_destroy,
	.transfer = device_transfer,
	.attach_irq = device_attach_irq,
};
// 构造函数实现
Device* device_create() {
    Device* obj = (Device*)os_malloc(sizeof(Device));
    if (obj) {
        memset(obj, 0, sizeof(Device));
        device_init(obj);
    }
    return obj;
}

void device_init(Device* self) {
    if (self->vtable == NULL) {
        self->vtable = (DeviceVTable *) os_malloc(sizeof(DeviceVTable));
        memset(self->vtable , 0, sizeof(DeviceVTable));
    }
    self->fun = &(device_fun);
    // TODO: 初始化数据成员
    self->semaphore = NULL;
}

void device_deinit(Device* self) {
    if (self->vtable != NULL) {
        os_free(self->vtable);
        self->vtable = NULL;
    }
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void device_destroy(Device* self) {
    if (self != NULL) {
        device_deinit(self);
        os_free(self);
    }
}

// attach_irq method
static bool device_attach_irq(Device* self, irq_config *conf, Semaphore * sem) {
    // TODO: add attach_irq method
    if (conf == NULL || conf->handler == NULL || conf->irq_num >= MAX_IRQ) {
        return false;
    }

    if (gloable_intc->fun->register_handler(gloable_intc, conf->irq_num, conf->handler, self)) {
        self->semaphore = sem;
        gloable_intc->fun->attach_semaphore(gloable_intc, conf->irq_num, self->semaphore);
        gloable_intc->fun->set_priority(gloable_intc, conf->irq_num, conf->priority);
        return true;
    }

    return false;
}

// transfer method
static bool device_transfer(Device* self, const void *data, size_t count) {
    if (self->vtable->dev_write == NULL) {
        return false;
    }
    self->vtable->dev_write(self, data, count);
    if (self->semaphore) {
        self->semaphore->fun->take(self->semaphore);
    }
    return true;
}




#include "device.h"
#include "../common/linear_pool.h"
#include <stdio.h>

static void device_redirect_semaphore(Device* self, Semaphore *sem);

static void device_attach_semaphore(Device* self, Semaphore *sem);

// 析构函数声明
static void device_destroy(Device* self);

// TODO: 初始化数据成员
static const DeviceFun device_fun = {
    .destroy = device_destroy,
	.attach_semaphore = device_attach_semaphore,
	.redirect_semaphore = device_redirect_semaphore,
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
        self->vtable = (DeviceVTable *) malloc(sizeof(DeviceVTable));
        memset(self->vtable , 0, sizeof(DeviceVTable));
    }
    self->fun = &(device_fun);
    // TODO: 初始化数据成员
    self->irq_num = MAX_IRQ;
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

// attach_semaphore method
static void device_attach_semaphore(Device* self, Semaphore *sem) {
    // TODO: add attach_semaphore method
    if (self == NULL) {
        return;
    }
    self->semaphore = sem;
    if (GET_DEVICE_VTABLE(self)->irq_handler != NULL && self->irq_num < MAX_IRQ) {
        gloable_intc->fun->register_handler(gloable_intc, self->irq_num, GET_DEVICE_VTABLE(self)->irq_handler, self);
        gloable_intc->fun->attach_semaphore(gloable_intc, self->irq_num, sem);
    }

}


// redirect_semaphore method
static void device_redirect_semaphore(Device* self, Semaphore *sem) {
    // TODO: add redirect_semaphore method
    if (self->semaphore == NULL) {
    self->semaphore = sem;
    }
    if (GET_DEVICE_VTABLE(self)->irq_handler != NULL && self->irq_num < MAX_IRQ) {
        gloable_intc->fun->register_handler(gloable_intc, self->irq_num, GET_DEVICE_VTABLE(self)->irq_handler, self);
        gloable_intc->fun->attach_semaphore(gloable_intc, self->irq_num, sem);
    }
}


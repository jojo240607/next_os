#include "device.h"
#include "../../common/linear_pool.h"
#include <stdio.h>

static uint32_t device_encode_pripority(Device* self, uint32_t peer_pripority, uint32_t sub_pripority);

static bool device_attach_irq(Device* self, irq_config *conf);

static bool device_transfer(Device* self, const void *data, size_t count);

// 析构函数声明
static void device_destroy(Device* self);

// TODO: 初始化数据成员
static const DeviceFun device_fun = {
    .destroy = device_destroy,
	.transfer = device_transfer,
	.attach_irq = device_attach_irq,
	.encode_pripority = device_encode_pripority,
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
    self->irq_conf.semaphore = NULL;
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
static bool device_attach_irq(Device* self, irq_config *conf) {
    // TODO: add attach_irq method
    if (conf == NULL || conf->handler == NULL || conf->irq_num >= MAX_IRQ) {
        return false;
    }

    if (gloable_nvic->fun->register_handler(gloable_nvic, conf->irq_num, conf->handler, self)) {
        gloable_nvic->fun->attach_semaphore(gloable_nvic, conf->irq_num, self->irq_conf.semaphore);
        gloable_nvic->fun->set_priority(gloable_nvic, conf->irq_num, conf->priority);
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
    if (self->irq_conf.semaphore) {
        self->irq_conf.semaphore->fun->take(self->irq_conf.semaphore);
    }
    return true;
}




// encode_pripority method
static uint32_t device_encode_pripority(Device* self, uint32_t peer_pripority, uint32_t sub_pripority) {
    return gloable_nvic->fun->encode_priority(gloable_nvic, peer_pripority, sub_pripority);
}


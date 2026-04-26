#include "bus.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"

dev_init_override(bus_dev_init_impl);

static bool bus_transfer_async(Bus* self, Device *device, const void *data, size_t count, transfer_done cb, void *arg);

static bool bus_transfer(Bus* self, Device *device, const void *data, size_t count);

// 析构函数声明
static void bus_destroy(Bus* self);

// TODO: 初始化数据成员
static const BusFun bus_fun = {
    .destroy = bus_destroy,
	.transfer = bus_transfer,
	.transfer_async = bus_transfer_async,
};
// 构造函数实现
Bus* bus_create() {
    Bus* obj = (Bus*)os_malloc(sizeof(Bus));
    if (obj) {
        memset(obj, 0, sizeof(Bus));
        bus_init(obj);
    }
    return obj;
}

void bus_init(Bus* self) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(bus_fun);
    // TODO: 初始化派生类特有成员

	def_dev_init(self) = bus_dev_init_impl;
}

void bus_deinit(Bus* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void bus_destroy(Bus* self) {
    if (self != NULL) {
        bus_deinit(self);
        os_free(self);
    }
}

// transfer method
static bool bus_transfer(Bus* self, Device *device, const void *data, size_t count) {
    if (self == NULL || device == NULL) {
        return false;
    }
    if (device->vtable->dev_write == NULL) {
        return false;
    }
    device->vtable->dev_write(device, data, count);
    if (device->semaphore) {
        device->semaphore->fun->take(device->semaphore);
    }
    return true;
}


// transfer_async method
static bool bus_transfer_async(Bus* self, Device *device, const void *data, size_t count, transfer_done cb, void *arg) {
    if (self == NULL || device == NULL) {
        return false;
    }
    if (device->vtable->dev_write == NULL) {
        return false;
    }
    device->vtable->dev_write(device, data, count);
    device->semaphore = GET_DEVICE(self)->semaphore;
    return true;
}


// dev_init method
dev_init_override(bus_dev_init_impl) {
    // TODO: add dev_init method
    Bus *bus = (Bus *)self;
    //params 
    self->semaphore = sem;
}


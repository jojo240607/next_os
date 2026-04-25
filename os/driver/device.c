#include "device.h"
#include "../common/linear_pool.h"
#include <stdio.h>

// 析构函数声明
static void device_destroy(Device* self);

// TODO: 初始化数据成员
static const DeviceFun device_fun = {
    .destroy = device_destroy,
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

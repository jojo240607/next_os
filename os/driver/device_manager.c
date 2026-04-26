#include "device_manager.h"
#include <stdio.h>
#include "usart.h"
#include "systick.h"
#include "bus.h"
#include "../common/linear_pool.h"

static Device* device_manager_dev_open(Device_manager* self, Dev_tag tag);

// 析构函数声明
static void device_manager_destroy(Device_manager* self);

// TODO: 初始化数据成员
static const Device_managerFun device_manager_fun = {
    .destroy = device_manager_destroy,
	.dev_open = device_manager_dev_open,
};

static const Device_list dev_lists[] = {
        {.tag = DEVICE_SYSTICK, .name = "sys_tick", .device_create = (Device_create) systick_create},
        {.tag = DEVICE_BUS, .name = "bus", .device_create = (Device_create) bus_create},
        {.tag = DEVICE_USART, .name = "usart", .device_create = (Device_create) usart_create},
};
Device_manager *gloable_deviceManager = NULL;
// 构造函数实现
Device_manager* device_manager_create() {
    Device_manager* obj = (Device_manager*)os_malloc(sizeof(Device_manager) + ARRAY_SIZE(dev_lists) * sizeof(Device *));
    if (obj) {
        memset(obj, 0, sizeof(Device_manager) + ARRAY_SIZE(dev_lists) * sizeof(Device *));
        device_manager_init(obj);
    }
    return obj;
}

void device_manager_init(Device_manager* self) {
    self->fun = &(device_manager_fun);
    // TODO: 初始化数据成员
    self->devicelist = dev_lists;
    self->dev_size =  ARRAY_SIZE(dev_lists);
}

void device_manager_deinit(Device_manager* self) {
    // TODO: 数据成员申请资源释放
    for (uint8_t i = 0; i < self->dev_size; i++) {
        if (self->dev_tab[i]) {
            self->dev_tab[i]->fun->destroy(self->dev_tab[i]);
            self->dev_tab[i] = NULL;
        }
    }
}

// 析构函数实现
static void device_manager_destroy(Device_manager* self) {
    if (self != NULL) {
        device_manager_deinit(self);
        os_free(self);
    }
}

// dev_open method
static Device* device_manager_dev_open(Device_manager* self, Dev_tag tag) {
    if (self == NULL) {
        return NULL;
    }
    if (tag < 0 || tag >= DEVICE_MAX) {
        return NULL;
    }
    if (self->dev_tab[tag] == NULL) {
        self->dev_tab[tag] = (self->devicelist + tag)->device_create();
    }
    return self->dev_tab[tag];
}


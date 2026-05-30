#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common/device.h"
#include "../common/util.h"
#include "hal/hal_svc.h"

#define GET_DEVICE_MANAGER(obj) ((Device_manager *)obj)
// 类声明
typedef struct _Device_manager Device_manager;
typedef struct _Device_managerFun Device_managerFun;
typedef struct _Device_list Device_list;



typedef Device * (*Device_create)(const device_info_t *info);
// 类成员函数结构


struct _Device_managerFun {
    void (*destroy)(Device_manager* self);
    Device* (*dev_open)(Device_manager* self, dev_id_t id);

};
struct _Device_list {
    const device_info_t *info;
    const Device_create device_create;
};

// 类结构
struct _Device_manager {
    const Device_managerFun* fun;
    // TODO: 添加数据成员
    const Device_list *devicelist;
    size_t dev_size;
    Device *dev_tab[];
};

// 构造函数声明
Device_manager* device_manager_create();
void device_manager_init(Device_manager* self);

// 析构函数声明
void device_manager_deinit(Device_manager* self);
extern Device_manager *gloable_deviceManager;
#endif // DEVICE_MANAGER_H
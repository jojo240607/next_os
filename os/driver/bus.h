#ifndef BUS_H
#define BUS_H
#include <stdint.h>
#include <stdbool.h>
#include "Device.h"

#define GET_BUS_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(BusVTable **)obj)
#define GET_BUS(obj) ((Bus *)obj)

// 派生类声明
typedef struct _Bus Bus;
typedef struct _BusFun BusFun;
typedef void (*transfer_done)(void *arg);
// 类成员函数结构
struct _BusFun {
    void (*destroy)(Bus* self);
	bool (*transfer)(Bus* self, Device *device, const void *data, size_t count);
	bool (*transfer_async)(Bus* self, Device *device, const void *data, size_t count, transfer_done cb, void *arg);

};
struct _Bus {
    Device base;  // 基类作为第一个成员
    const BusFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Bus* bus_create();
void bus_init(Bus* self);

// 析构函数声明
void bus_deinit(Bus* self);

#endif // BUS_H
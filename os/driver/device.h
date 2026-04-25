#ifndef DEVICE_H
#define DEVICE_H
/*
 *
override void dev_init();
override void dev_read(void *buf, size_t count);
override void dev_write(const void *buf, size_t count);
override void dev_ioctl(int cmd, void *arg);
override void irq_handler(void *arg);
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "intc.h"

#define GET_DEVICE_VTABLE(obj) (*(DeviceVTable **)obj)

#define dev_init_override(func_name) static void func_name(Device* self, Semaphore *sem)
#define def_dev_init(obj) (GET_DEVICE_VTABLE(obj)->dev_init)
#define virtual_dev_init(obj, ...) def_dev_init(obj)(obj, ##__VA_ARGS__)

#define dev_read_override(func_name) static void func_name(Device* self, void *buf, size_t count)
#define def_dev_read(obj) (GET_DEVICE_VTABLE(obj)->dev_read)
#define virtual_dev_read(obj, ...) def_dev_read(obj)(obj, ##__VA_ARGS__)

#define dev_write_override(func_name) static void func_name(Device* self, const void *buf, size_t count)
#define def_dev_write(obj) (GET_DEVICE_VTABLE(obj)->dev_write)
#define virtual_dev_write(obj, ...) def_dev_write(obj)(obj, ##__VA_ARGS__)

#define dev_ioctl_override(func_name) static void func_name(Device* self, int cmd, void *arg)
#define def_dev_ioctl(obj) (GET_DEVICE_VTABLE(obj)->dev_ioctl)
#define virtual_dev_ioctl(obj, ...) def_dev_ioctl(obj)(obj, ##__VA_ARGS__)

#define irq_handler_override(func_name) static bool func_name(void *arg)
#define def_irq_handler(obj) (GET_DEVICE_VTABLE(obj)->irq_handler)
#define virtual_irq_handler(obj, ...) def_irq_handler(obj)(obj, ##__VA_ARGS__)

#define GET_DEVICE(obj) ((Device *)obj)
// 类声明
typedef struct _Device Device;
typedef struct _DeviceFun DeviceFun;
typedef struct _DeviceVTable DeviceVTable;

// 虚函数表结构
typedef struct _DeviceVTable {
    // TODO : 添加其他虚函数

	void (*dev_init)(Device* self, Semaphore *sem);
	void (*dev_read)(Device* self, void *buf, size_t count);
	void (*dev_write)(Device* self, const void *buf, size_t count);
	void (*dev_ioctl)(Device* self, int cmd, void *arg);
    
	bool (*irq_handler)(void *arg);

};
// 类成员函数结构
struct _DeviceFun {
    void (*destroy)(Device* self);
};
// 类结构
struct _Device {
    DeviceVTable* vtable;
    const DeviceFun* fun;
    // TODO: 添加数据成员
    //void (*irq_handler)(Real_time_task* self);
};

// 构造函数声明
Device* device_create();
void device_init(Device* self);

// 析构函数声明
void device_deinit(Device* self);

#endif // DEVICE_H
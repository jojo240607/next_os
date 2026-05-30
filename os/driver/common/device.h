#ifndef DEVICE_H
#define DEVICE_H
/*
 *
override void dev_init();
override void dev_read(void *buf, size_t count);
override void dev_write(const void *buf, size_t count);
override void dev_ioctl(ioctl_cmd_t cmd, void *arg);

 
void dev_read_user(void *buf, size_t count);
void dev_write_user(const void *buf, size_t count);
void dev_ioctl_user(int cmd, void *arg);
 
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nvic.h"
#include "../hal/hal_nvic.h"
#include "../../common/list.h"

#define GET_DEVICE_VTABLE(obj) (*(DeviceVTable **)obj)

#define dev_init_override(func_name) static void func_name(Device* self)
#define def_dev_init(obj) (GET_DEVICE_VTABLE(obj)->dev_init)
#define virtual_dev_init(obj, ...) def_dev_init(obj)(obj, ##__VA_ARGS__)

#define dev_read_override(func_name) static void func_name(Device* self, void *buf, size_t count)
#define def_dev_read(obj) (GET_DEVICE_VTABLE(obj)->dev_read)
#define virtual_dev_read(obj, ...) def_dev_read(obj)(obj, ##__VA_ARGS__)

#define dev_write_override(func_name) static void func_name(Device* self, const void *buf, size_t count)
#define def_dev_write(obj) (GET_DEVICE_VTABLE(obj)->dev_write)
#define virtual_dev_write(obj, ...) def_dev_write(obj)(obj, ##__VA_ARGS__)

#define dev_ioctl_override(func_name) static void func_name(Device* self, ioctl_cmd_t cmd, void *arg)
#define def_dev_ioctl(obj) (GET_DEVICE_VTABLE(obj)->dev_ioctl)
#define virtual_dev_ioctl(obj, ...) def_dev_ioctl(obj)(obj, ##__VA_ARGS__)

#define GET_DEVICE(obj) ((Device *)obj)
// 类声明
typedef struct _Device Device;
typedef struct _DeviceFun DeviceFun;
typedef struct _DeviceVTable DeviceVTable;
typedef struct _irq_config irq_config;

typedef enum : uint8_t {
    DEVICE_SYSTICK = 0,
    DEVICE_USART4,
    DEVICE_USART1,
    DEVICE_TIME2,
    DEVICE_EXTI,
    DEVICE_ADC1,
    DEVICE_SPI1,
    DEVICE_I2C1,
    DEVICE_WDG,
    DEVICE_CAN,
    DEVICE_PWM1,
    DEVICE_I2S2,
    DEVICE_MAX
} dev_id_t;

typedef enum :uint8_t {
    DEVICE_READ = 0,
    DEVICE_WRITE,
    DEVICE_IOCTL,
} device_cmd;

typedef struct {
    device_cmd cmd;
    void *buf;
    size_t count;
} device_ctrl;
typedef enum :uint8_t {
    DEVICE_START = 0x30,
    DEVICE_STOP,
} ioctl_cmd_t;
// 虚函数表结构
struct _DeviceVTable {
    // TODO : 添加其他虚函数
	void (*dev_init)(Device* self);
	void (*dev_read)(Device* self, void *buf, size_t count);
	void (*dev_write)(Device* self, const void *buf, size_t count);
	void (*dev_ioctl)(Device* self, ioctl_cmd_t cmd, void *arg);
};

typedef struct {
    nvic_priority_t peer_pripority;
    uint8_t sub_pripority;
} dev_pripority_t;

struct _irq_config {
    List *irq_list; //irq_node_t
    void *arg;
    void *event;
    nvic_handler_t handler;
    const dev_pripority_t *priority;
};

// 类成员函数结构
struct _DeviceFun {
    void (*destroy)(Device* self);
	bool (*transfer)(Device* self, const void *data, size_t count);
    bool (*attach_irq)(Device* self, Task *task);

	void (*read_user)(Device* self, void *buf, size_t count);
	void (*write_user)(Device* self, const void *buf, size_t count);
	void (*ioctl_user)(Device* self, int cmd, void *arg);

    bool (*config_irq)(Device* self, const irq_config *conf);

	void (*add_event)(Device* self, void *event);

};
typedef struct {
    const char *name;
    dev_id_t id;
    const dev_pripority_t *priority;
    const void *conf;
} device_info_t;
// 类结构
struct _Device {
    DeviceVTable* vtable;
    const DeviceFun* fun;
    // TODO: 添加数据成员
    const device_info_t *info;
    irq_config *irq_conf;
};

// 构造函数声明
Device* device_create(const device_info_t *info);
void device_init(Device* self, const device_info_t *info);

// 析构函数声明
void device_deinit(Device* self);

#endif // DEVICE_H
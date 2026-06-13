#include "device.h"
#include "../../common/linear_pool.h"
#include "../svc.h"
#include <stdio.h>

static void device_trigger_event(Device* self, uint8_t event, void *arg);

static void device_register_listener(Device* self, dev_listener listener);

static void device_add_event(Device* self, void *event);

static bool device_config_irq(Device* self, const irq_config *conf);

static void device_dev_read_user(Device* self, void *buf, size_t count);
static void device_dev_write_user(Device* self, const void *buf, size_t count);
static void device_dev_ioctl_user(Device* self, ioctl_cmd_t cmd, void *arg);

static bool device_attach_irq(Device* self, Task *task);

static bool device_transfer(Device* self, const void *data, size_t count);

// 析构函数声明
static void device_destroy(Device* self);

// TODO: 初始化数据成员
static const DeviceFun device_fun = {
    .destroy = device_destroy,
	.transfer = device_transfer,
	.attach_irq = device_attach_irq,
	.read_user = device_dev_read_user,
	.write_user = device_dev_write_user,
	.ioctl_user = device_dev_ioctl_user,
	.config_irq = device_config_irq,
	.add_event = device_add_event,
	.register_listener = device_register_listener,
	.trigger_event = device_trigger_event,
};

// 构造函数实现
Device* device_create(const device_info_t *info) {
    Device* obj = (Device*)os_malloc(sizeof(Device));
    if (obj) {
        memset(obj, 0, sizeof(Device));
        device_init(obj, info);
    }
    return obj;
}

void device_init(Device* self, const device_info_t *info) {
    if (self->vtable == NULL) {
        self->vtable = (DeviceVTable *) os_malloc(sizeof(DeviceVTable));
        memset(self->vtable , 0, sizeof(DeviceVTable));
    }
    self->fun = &(device_fun);
    // TODO: 初始化数据成员
    self->info = info;
    if (info->priority != NULL) {
        self->irq_conf = os_malloc(sizeof(irq_config));
        memset(self->irq_conf, 0, sizeof(irq_config));
        self->irq_conf->irq_list = list_create();
        self->irq_conf->priority = info->priority;
    } else {
        self->irq_conf = NULL;
    }
    self->read_mutex = mutex_create();
    self->write_mutex = mutex_create();
    self->listener = NULL;
    self->arg = NULL;
}

void device_deinit(Device* self) {
    if (self->vtable != NULL) {
        os_free(self->vtable);
        self->vtable = NULL;
    }
    // TODO: 数据成员申请资源释放
    if (self->irq_conf != NULL) {
        if (self->irq_conf->irq_list) {
            self->irq_conf->irq_list->fun->destroy(self->irq_conf->irq_list);
        }
        os_free(self->irq_conf);
    }
}

// 析构函数实现
static void device_destroy(Device* self) {
    if (self != NULL) {
        device_deinit(self);
        os_free(self);
    }
}

// attach_irq method
// 将设备的所有已配置 IRQ 绑定到指定任务的下半部处理。
// 注意：irq_conf->handler 可能被子类多次修改（DMA TX/RX 各自设置），
// 但 attach_irq 只关心 bottom_task 绑定，不依赖当前的 handler 值。
// TODO: 将 handler 和 bottom_task 分离到独立结构，消除 handler 判空对绑定的干扰。
static bool device_attach_irq(Device* self, Task *task) {
    // TODO: add attach_irq method
    if (self->irq_conf == NULL || self->irq_conf->irq_list == NULL || self->irq_conf->irq_list->size == 0) {
        return false;
    }
    for (uint8_t i = 0; i < self->irq_conf->irq_list->size; i++) {
        nvic_irq_num irq_num = self->irq_conf->irq_list->fun->at_int(self->irq_conf->irq_list, i);
        if (irq_num < MAX_IRQ) {
            nvic_attach_task(gloable_nvic, irq_num, task);
        }
    }
    return true;
}

// transfer method
static bool device_transfer(Device* self, const void *data, size_t count) {
    if (self->vtable->dev_write == NULL) {
        return false;
    }
    self->vtable->dev_write(self, data, count);
    return true;
}



// dev_read_user method
static void device_dev_read_user(Device* self, void *buf, size_t count) {
    const device_ctrl ctrl = {
            .cmd = DEVICE_READ,
            .buf = buf,
            .count = count,
    };
    mutex_lock_user(self->read_mutex, 1000);     //添加互斥锁，防止多线程调用
    device_user(self, &ctrl);
    mutex_unlock_user(self->read_mutex);     //添加互斥锁，防止多线程调用

}
// dev_write_user method
static void device_dev_write_user(Device* self, const void *buf, size_t count) {
    const device_ctrl ctrl = {
            .cmd = DEVICE_WRITE,
            .buf = (void *)buf,
            .count = count,
    };
    mutex_lock_user(self->write_mutex, 1000);     //添加互斥锁，防止多线程调用
    device_user(self, &ctrl);
    mutex_unlock_user(self->write_mutex);     //添加互斥锁，防止多线程调用
}
// dev_ioctl_user method
static void device_dev_ioctl_user(Device* self, ioctl_cmd_t cmd, void *arg) {

    device_transfer_conf_t trans_conf = {.cmd = cmd,
                                         .conf = arg};
    const device_ctrl ctrl = {
            .cmd = DEVICE_IOCTL,
            .buf = &trans_conf,
    };
    mutex_lock_user(self->write_mutex, 1000);     //添加互斥锁，防止多线程调用
    device_user(self, &ctrl);
    mutex_unlock_user(self->write_mutex);     //添加互斥锁，防止多线程调用
}


// config_irq method
static bool device_config_irq(Device* self, const irq_config *conf) {
    // TODO: add config_irq method
    if (self->irq_conf == NULL || conf == NULL || conf->handler == NULL) {
        return false;
    }
    for (uint32_t i = 0; i < self->irq_conf->irq_list->size; i++) {
        nvic_irq_num irq_num = (self->irq_conf->irq_list->fun->at_int(self->irq_conf->irq_list, i) & 0xff);
        if (irq_num < MAX_IRQ) {
            if (nvic_register(gloable_nvic, irq_num, conf->handler, self, conf->event)) {
                nvic_set_priority(gloable_nvic, irq_num, self->irq_conf->priority->peer_pripority, self->irq_conf->priority->sub_pripority);
            }
        }
    }
    return true;
}


// add_event method
static void device_add_event(Device* self, void *event) {
    // TODO: add add_event method
    if (!event) {
        return;
    }
    //将消息event传递到irq conf中
    self->irq_conf->event = event;
}


// register_listener method
static void device_register_listener(Device* self, dev_listener listener) {
    // 创建链表节点，追加到 listener 链表
    listener_node_t *node = (listener_node_t *)os_malloc(sizeof(listener_node_t));
    if (node) {
        node->next = self->listener;
        node->fn   = listener;
        self->listener = node;
    }
}


// trigger_event method
static void device_trigger_event(Device* self, uint8_t event, void *arg) {
    self->current_event = event;
    listener_node_t *n = self->listener;
    while (n) {
        n->fn(self, event, arg);
        n = n->next;
    }
}


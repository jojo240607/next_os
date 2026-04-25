#include "usart.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>

dev_init_override(usart_dev_init_impl);
dev_read_override(usart_dev_read_impl);
dev_write_override(usart_dev_write_impl);
dev_ioctl_override(usart_dev_ioctl_impl);

// 析构函数声明
static void usart_destroy(Usart* self);

// TODO: 初始化数据成员
static const UsartFun usart_fun = {
    .destroy = usart_destroy,
};
// 构造函数实现
Usart* usart_create() {
    Usart* obj = (Usart*)os_malloc(sizeof(Usart));
    if (obj) {
        memset(obj, 0, sizeof(Usart));
        usart_init(obj);
    }
    return obj;
}

void usart_init(Usart* self) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(usart_fun);
    // TODO: 初始化派生类特有成员

	def_dev_init(self) = usart_dev_init_impl;
	def_dev_read(self) = usart_dev_read_impl;
	def_dev_write(self) = usart_dev_write_impl;
	def_dev_ioctl(self) = usart_dev_ioctl_impl;
}

void usart_deinit(Usart* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void usart_destroy(Usart* self) {
    if (self != NULL) {
        usart_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(usart_dev_init_impl) {
    // TODO: add dev_init method
    Usart *usart = (Usart *)self;
    //params 
    
}
// dev_read method
dev_read_override(usart_dev_read_impl) {
    // TODO: add dev_read method
    Usart *usart = (Usart *)self;
    //params , void *buf, size_t count
    
}
// dev_write method
dev_write_override(usart_dev_write_impl) {
    // TODO: add dev_write method
    Usart *usart = (Usart *)self;
    //params , const void *buf, size_t count
    
}
// dev_ioctl method
dev_ioctl_override(usart_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Usart *usart = (Usart *)self;
    //params , int cmd, void *arg
    
}



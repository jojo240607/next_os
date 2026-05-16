#ifndef EXTI_H
#define EXTI_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/pinmux.h"

#define GET_EXTI_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(ExtiVTable **)obj)
#define GET_EXTI(obj) ((Exti *)obj)

// 派生类声明
typedef struct _Exti Exti;
typedef struct _ExtiFun ExtiFun;
typedef struct _exti_config exti_config;


// 类成员函数结构
struct _ExtiFun {
    void (*destroy)(Exti* self);
};
/* ADC 通道描述符 */
typedef struct {
    gpio_port_t port;
    uint8_t     pin;
    exti_mode   exti_mode;
} exti_pin_cfg_t;
struct _exti_config {
    uint8_t pin_size;
    const exti_pin_cfg_t * const pin_conf[];
};
struct _Exti {
    Device base;  // 基类作为第一个成员
    const ExtiFun* fun;
    // TODO: 添加派生类特有的数据成员
    const exti_config *conf;
};

// 构造函数声明
Exti* exti_create(const exti_config *conf);
void exti_init(Exti* self, const exti_config *conf);

// 析构函数声明
void exti_deinit(Exti* self);
bool exti_irq_handler_impl(void *arg);

#endif // EXTI_H
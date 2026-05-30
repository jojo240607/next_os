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

typedef enum : uint8_t {
    EVENT_SOURCE_EXTI0 = 0xe0,
    EVENT_SOURCE_EXTI1,
    EVENT_SOURCE_EXTI2,
    EVENT_SOURCE_EXTI3,
    EVENT_SOURCE_EXTI4,
    EVENT_SOURCE_EXTI5,
    EVENT_SOURCE_EXTI6,
    EVENT_SOURCE_EXTI7,
    EVENT_SOURCE_EXTI8,
    EVENT_SOURCE_EXTI9,
    EVENT_SOURCE_EXTI10,
    EVENT_SOURCE_EXTI11,
    EVENT_SOURCE_EXTI12,
    EVENT_SOURCE_EXTI13,
    EVENT_SOURCE_EXTI14,
    EVENT_SOURCE_EXTI15,
} event_source_t;

typedef struct {
    nvic_irq_num irq_num;    //nvic_irq_num
    event_source_t source;
} exti_event_t;

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
typedef struct {
    uint8_t pin_size;
    const exti_pin_cfg_t * const pin_conf[];
} exti_config_t;
struct _Exti {
    Device base;  // 基类作为第一个成员
    const ExtiFun* fun;
    // TODO: 添加派生类特有的数据成员
    exti_event_t *exti_event;
};

// 构造函数声明
Exti* exti_create(const device_info_t *info);
void exti_init(Exti* self, const device_info_t *info);

// 析构函数声明
void exti_deinit(Exti* self);
bool exti_irq_handler_impl(nvic_irq_t *irq_conf);

#endif // EXTI_H
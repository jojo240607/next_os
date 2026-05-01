#ifndef PINMUX_H
#define PINMUX_H

#include <stdint.h>
#include <stdbool.h>
#include "device.h"

#define PINMUX_ERROR 0
#define PINMUX_SUCCESS 1
typedef enum _pin_mode pin_mode;
typedef enum _pin_otype pin_otype;
typedef enum _pin_ospeed pin_ospeed;
typedef enum _pin_pupd pin_pupd;
typedef enum _exti_mode exti_mode;
/* 端口枚举，顺序对应 RCC_AHB1ENR 的 bit 位 */
typedef enum {
    PORT_A = 0,
    PORT_B,
    PORT_C,
    PORT_D,
    PORT_E,
    PORT_F,
    PORT_G,
    PORT_H,
    PORT_I,
    PORT_MAX
} gpio_port_t;

/* 工作模式 */
enum _pin_mode {
    PIN_MODE_INPUT = 0x00,
    PIN_MODE_OUTPUT,
    PIN_MODE_AF,
    PIN_MODE_ANALOG
};

enum  _pin_otype {
    PIN_OTYPE_PP = 0, /* 推挽 */
    PIN_OTYPE_OD  /* 开漏 */
};
/* 输出速度 */
enum  _pin_ospeed {
    PIN_OSPEED_LOW = 0,
    PIN_OSPEED_MEDIUM,
    PIN_OSPEED_HIGH,
    PIN_OSPEED_VERY_HIGH
};

/* 上下拉 */
enum  _pin_pupd {
    PIN_PUPD_NONE = 0,
    PIN_PUPD_PULLUP,
    PIN_PUPD_PULLDOWN,
};

/* 新增：中断触发类型 */
enum _exti_mode {
    PIN_IRQ_MODE_NONE = 0,  // 无中断
    PIN_IRQ_MODE_RISING,    // 上升沿
    PIN_IRQ_MODE_FALLING,   // 下降沿
    PIN_IRQ_MODE_BOTH       // 双边沿
};

/* 引脚配置描述符 */
typedef struct {
    gpio_port_t port;
    uint8_t     pin;       /* 0..15 */
    pin_mode     mode;      /* PIN_MODE_xxx */
    pin_otype     otype;     /* PIN_OTYPE_xx */
    pin_ospeed     ospeed;    /* PIN_OSPEED_xx */
    pin_pupd     pupd;      /* PIN_PUPD_xx */
    uint8_t     af;        /* 复用功能编号 0..15 */
    /* 中断扩展字段 */
    exti_mode     irq_mode;  /* PIN_IRQ_MODE_NONE / RISING / FALLING / BOTH */
} pin_config_t;

/* API 函数 */
void pinmux_init(void);
int  pinmux_request(const pin_config_t *cfg, const irq_config *irq_cfg);
int  pinmux_release(gpio_port_t port, uint8_t pin);
int  pinmux_request_group(const pin_config_t *cfgs, int count, const irq_config *irq_cfg);

#endif
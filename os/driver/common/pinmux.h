#ifndef PINMUX_H
#define PINMUX_H

#include <stdint.h>
#include <stdbool.h>
#include "device.h"
#include "../hal/hal_gpio.h"

#define PINMUX_ERROR 0
#define PINMUX_SUCCESS 1

/* 引脚配置描述符 */
typedef struct {
    gpio_port_t     port;
    gpio_pin_t      pin;       /* 0..15 */
    pin_mode        mode;      /* PIN_MODE_xxx */
    pin_otype       otype;     /* PIN_OTYPE_xx */
    pin_ospeed      ospeed;    /* PIN_OSPEED_xx */
    pin_pupd        pupd;      /* PIN_PUPD_xx */
    pin_af          af;        /* 复用功能编号 0..15 */
    /* 中断扩展字段 */
    exti_mode       irq_mode;  /* PIN_IRQ_MODE_NONE / RISING / FALLING / BOTH */
} pin_config_t;


/* API 函数 */
void pinmux_init(void);
int  pinmux_request(const pin_config_t *cfg);
int  pinmux_release(gpio_port_t port, uint8_t pin);
int  pinmux_request_group(const pin_config_t *cfgs, int count);

#endif
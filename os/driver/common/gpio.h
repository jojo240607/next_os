#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "pinmux.h"       // 使用已有的 GPIO 寄存器定义和端口枚举
#include "../hal/hal_gpio.h"


/* GPIO 引脚描述符 */
typedef struct {
    gpio_port_t port;     // 端口 (PORT_A ~ PORT_I)
    gpio_pin_t    pin;      // 引脚号 (0..15)
} gpio_t;

/* API 函数 */
void gpio_set(const gpio_t *gpio_conf);          // 置高
void gpio_reset(const gpio_t *gpio_conf);        // 置低
void gpio_write(const gpio_t *gpio_conf, bool value); // 写电平
void gpio_toggle(const gpio_t *gpio_conf);       // 翻转
bool gpio_read(const gpio_t *gpio_conf);         // 读输入电平
void gpio_lock(const gpio_t *gpio_conf);         // 锁定配置

/* 便捷宏：位带操作地址 (用于极致性能场景) */

#define GPIO_BITBAND_BASE   0x42000000UL
#define GPIO_BITBAND_ADDR(port, pin, reg_offset) \
    (GPIO_BITBAND_BASE + ((uint32_t)&GPIOx[port]->reg_offset - 0x40000000UL) * 32 + (pin) * 4)

#endif /* GPIO_H */
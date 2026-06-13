/**
 * gpio.c — GPIO 原子操作封装
 * 所有寄存器访问通过 hal_gpio_pin_* 函数。
 */
#include "gpio.h"
#include "../hal/hal_gpio.h"

void gpio_set(const gpio_t *gpio_conf)
{
    hal_gpio_pin_set(gpio_conf->port, gpio_conf->pin);
}

void gpio_reset(const gpio_t *gpio_conf)
{
    hal_gpio_pin_reset(gpio_conf->port, gpio_conf->pin);
}

void gpio_write(const gpio_t *gpio_conf, bool value)
{
    hal_gpio_pin_write(gpio_conf->port, gpio_conf->pin, value);
}

void gpio_toggle(const gpio_t *gpio_conf)
{
    hal_gpio_pin_toggle(gpio_conf->port, gpio_conf->pin);
}

bool gpio_read(const gpio_t *gpio_conf)
{
    return hal_gpio_pin_read(gpio_conf->port, gpio_conf->pin);
}

void gpio_lock(const gpio_t *gpio_conf)
{
    hal_gpio_pin_lock(gpio_conf->port, gpio_conf->pin);
}

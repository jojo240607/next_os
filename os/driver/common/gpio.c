#include "gpio.h"
#include "../hal/hal_gpio.h"

/* ---------- 原子置位/复位 (基于 BSRR) ---------- */
void gpio_set(const gpio_t *gpio_conf)
{
    if (gpio_conf->port < PORT_MAX && gpio_conf->pin < PIN_MAX) {
        GPIOx[gpio_conf->port]->BSRR = (1U << gpio_conf->pin);   // 低16位写1置位
    }
}

void gpio_reset(const gpio_t *gpio_conf)
{
    if (gpio_conf->port < PORT_MAX && gpio_conf->pin < PIN_MAX) {
        GPIOx[gpio_conf->port]->BSRR = (1U << (gpio_conf->pin + 16)); // 高16位写1复位
    }
}

void gpio_write(const gpio_t *gpio_conf, bool value)
{
    if (value)
        gpio_set(gpio_conf);
    else
        gpio_reset(gpio_conf);
}

/* ---------- 翻转 (读 ODR 并写回取反) ---------- */
void gpio_toggle(const gpio_t *gpio_conf)
{
    if (gpio_conf->port < PORT_MAX && gpio_conf->pin < PIN_MAX) {
        uint32_t odr = GPIOx[gpio_conf->port]->ODR;
        if (odr & (1U << gpio_conf->pin))
            gpio_reset(gpio_conf);
        else
            gpio_set(gpio_conf);
    }
}

/* ---------- 读取输入电平 ---------- */
bool gpio_read(const gpio_t *gpio_conf)
{
    if (gpio_conf->port < PORT_MAX && gpio_conf->pin < 16) {
        return (GPIOx[gpio_conf->port]->IDR & (1U << gpio_conf->pin)) != 0;
    }
    return false;
}

/* ---------- 锁定配置 (LCKR) ---------- */
void gpio_lock(const gpio_t *gpio_conf)
{
    if (gpio_conf->port < PORT_MAX && gpio_conf->pin < 16) {
        volatile uint32_t *lckr = &GPIOx[gpio_conf->port]->LCKR;
        uint32_t mask = (1U << gpio_conf->pin);
        // 锁定序列: WR LCKR = (mask | 1<<16), 然后 WR LCKR = mask, 再 WR LCKR = (mask | 1<<16), 读 LCKR
        *lckr = mask | (1U << 16);
        *lckr = mask;
        *lckr = mask | (1U << 16);
        (void)*lckr;
        (void)*lckr;
    }
}
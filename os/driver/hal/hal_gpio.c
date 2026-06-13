//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_gpio.h"
#include "../common/rcc.h"

/* 端口基址查找表 */
xGPIO_TypeDef* const GPIOx[] = {
        (xGPIO_TypeDef*)xGPIOA_BASE,
        (xGPIO_TypeDef*)xGPIOB_BASE,
        (xGPIO_TypeDef*)xGPIOC_BASE,
        (xGPIO_TypeDef*)xGPIOD_BASE,
        (xGPIO_TypeDef*)xGPIOE_BASE,
        (xGPIO_TypeDef*)xGPIOF_BASE,
        (xGPIO_TypeDef*)xGPIOG_BASE,
        (xGPIO_TypeDef*)xGPIOH_BASE,
        (xGPIO_TypeDef*)xGPIOI_BASE
};

/* ---------- 硬件抽象层 (底层寄存器操作) ---------- */
void hal_gpio_clock_enable(gpio_port_t port)
{
    if (port < PORT_MAX) {
        rcc_periph_clock_enable(RCC_BUS_AHB1, xRCC_AHB1ENR_GPIOAEN + port);
        __asm volatile ("dsb" ::: "memory"); /* 确保总线可见 */
    }
}

void hal_gpio_set_mode(xGPIO_TypeDef *gpiox, uint8_t pin, pin_mode mode)
{
    uint32_t temp = gpiox->MODER;
    temp &= ~(3U << (pin << 1));
    temp |= (mode & 3U) << (pin << 1);
    gpiox->MODER = temp;
}

void hal_gpio_set_af(xGPIO_TypeDef *gpiox, uint8_t pin, uint8_t af)
{
    if (pin < 8) {
        uint32_t temp = gpiox->AFRL;
        temp &= ~(0xFU << (pin << 2));
        temp |= (af & 0xFU) << (pin << 2);
        gpiox->AFRL = temp;
    } else {
        uint32_t temp = gpiox->AFRH;
        temp &= ~(0xFU << ((pin - 8) << 2));
        temp |= (af & 0xFU) << ((pin - 8) << 2);
        gpiox->AFRH = temp;
    }
}

void hal_gpio_set_otype(xGPIO_TypeDef *gpiox, uint8_t pin, pin_otype otype)
{
    if (otype) {
        gpiox->OTYPER |= (1U << pin);
    } else {
        gpiox->OTYPER &= ~(1U << pin);
    }
}

void hal_gpio_set_ospeed(xGPIO_TypeDef *gpiox, uint8_t pin, pin_ospeed ospeed)
{
    uint32_t temp = gpiox->OSPEEDR;
    temp &= ~(3U << (pin << 1));
    temp |= (ospeed & 3U) << (pin << 1);
    gpiox->OSPEEDR = temp;
}

void hal_gpio_set_pupd(xGPIO_TypeDef *gpiox, uint8_t pin, pin_pupd pupd)
{
    uint32_t temp = gpiox->PUPDR;
    temp &= ~(3U << (pin << 1));
    temp |= (pupd & 3U) << (pin << 1);
    gpiox->PUPDR = temp;
}

/* ═══════════════════ 原子引脚操作 ═══════════════════ */

void hal_gpio_pin_set(gpio_port_t port, gpio_pin_t pin)
{
    if (port < PORT_MAX && pin < PIN_MAX) {
        GPIOx[port]->BSRR = (1U << pin);
    }
}

void hal_gpio_pin_reset(gpio_port_t port, gpio_pin_t pin)
{
    if (port < PORT_MAX && pin < PIN_MAX) {
        GPIOx[port]->BSRR = (1U << (pin + 16));
    }
}

void hal_gpio_pin_write(gpio_port_t port, gpio_pin_t pin, bool value)
{
    if (value)
        hal_gpio_pin_set(port, pin);
    else
        hal_gpio_pin_reset(port, pin);
}

void hal_gpio_pin_toggle(gpio_port_t port, gpio_pin_t pin)
{
    if (port < PORT_MAX && pin < PIN_MAX) {
        uint32_t odr = GPIOx[port]->ODR;
        if (odr & (1U << pin))
            hal_gpio_pin_reset(port, pin);
        else
            hal_gpio_pin_set(port, pin);
    }
}

bool hal_gpio_pin_read(gpio_port_t port, gpio_pin_t pin)
{
    if (port < PORT_MAX && pin < PIN_MAX) {
        return (GPIOx[port]->IDR & (1U << pin)) != 0;
    }
    return false;
}

void hal_gpio_pin_lock(gpio_port_t port, gpio_pin_t pin)
{
    if (port < PORT_MAX && pin < PIN_MAX) {
        volatile uint32_t *lckr = &GPIOx[port]->LCKR;
        uint32_t mask = (1U << pin);
        *lckr = mask | (1U << 16);
        *lckr = mask;
        *lckr = mask | (1U << 16);
        (void)*lckr;
        (void)*lckr;
    }
}

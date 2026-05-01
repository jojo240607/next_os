#include "pinmux.h"
#include "stm32f407xx.h"
#include "device.h"
#include "../log/log.h"
#include <stddef.h>

/* ---------- 寄存器定义（仅需用到的部分） ---------- */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;    /* 偏移 0x20, 引脚 0..7 */
    volatile uint32_t AFRH;    /* 偏移 0x24, 引脚 8..15 */
} xGPIO_TypeDef;

/* GPIO 基址 */
#define xGPIOA_BASE 0x40020000UL
#define xGPIOB_BASE 0x40020400UL
#define xGPIOC_BASE 0x40020800UL
#define xGPIOD_BASE 0x40020C00UL
#define xGPIOE_BASE 0x40021000UL
#define xGPIOF_BASE 0x40021400UL
#define xGPIOG_BASE 0x40021800UL
#define xGPIOH_BASE 0x40021C00UL
#define xGPIOI_BASE 0x40022000UL

/* 端口基址查找表 */
static xGPIO_TypeDef* const GPIOx[] = {
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

/* RCC 简化 */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
} xRCC_TypeDef;

#define xRCC_BASE 0x40023800UL
#define xRCC ((xRCC_TypeDef*)xRCC_BASE)
/* SYSCFG 基地址 */
#define xSYSCFG_BASE 0x40013800UL
typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t PMC;
    volatile uint32_t EXTICR[4];   // 四个寄存器，每个管理4个EXTI线
    // ... 其他寄存器忽略
} xSYSCFG_TypeDef;
#define xSYSCFG ((xSYSCFG_TypeDef*)xSYSCFG_BASE)

/* EXTI 基地址 */
#define xEXTI_BASE 0x40013C00UL
typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} xEXTI_TypeDef;
#define xEXTI ((xEXTI_TypeDef*)xEXTI_BASE)

/* ---------- 引脚状态管理 ---------- */
typedef struct {
    bool        allocated;
    pin_config_t config;
} pin_state_t;

static pin_state_t pin_states[PORT_MAX][16];
static bool clock_enabled[PORT_MAX] = {false};

/* ---------- 硬件抽象层 (底层寄存器操作) ---------- */
static void hal_gpio_clock_enable(gpio_port_t port)
{
    if (!clock_enabled[port]) {
        xRCC->AHB1ENR |= (1U << port);
        clock_enabled[port] = true;
        __asm volatile ("dsb" ::: "memory"); /* 确保总线可见 */
    }
}

static void hal_gpio_set_mode(xGPIO_TypeDef *GPIOx, uint8_t pin, pin_mode mode)
{
    uint32_t temp = GPIOx->MODER;
    temp &= ~(3U << (pin << 1));
    temp |= (mode & 3U) << (pin << 1);
    GPIOx->MODER = temp;
}

static void hal_gpio_set_af(xGPIO_TypeDef *GPIOx, uint8_t pin, uint8_t af)
{
    if (pin < 8) {
        uint32_t temp = GPIOx->AFRL;
        temp &= ~(0xFU << (pin << 2));
        temp |= (af & 0xFU) << (pin << 2);
        GPIOx->AFRL = temp;
    } else {
        uint32_t temp = GPIOx->AFRH;
        temp &= ~(0xFU << ((pin - 8) << 2));
        temp |= (af & 0xFU) << ((pin - 8) << 2);
        GPIOx->AFRH = temp;
    }
}

static void hal_gpio_set_otype(xGPIO_TypeDef *GPIOx, uint8_t pin, pin_otype otype)
{
    if (otype) {
        GPIOx->OTYPER |= (1U << pin);
    } else {
        GPIOx->OTYPER &= ~(1U << pin);
    }
}

static void hal_gpio_set_ospeed(xGPIO_TypeDef *GPIOx, uint8_t pin, pin_ospeed ospeed)
{
    uint32_t temp = GPIOx->OSPEEDR;
    temp &= ~(3U << (pin << 1));
    temp |= (ospeed & 3U) << (pin << 1);
    GPIOx->OSPEEDR = temp;
}

static void hal_gpio_set_pupd(xGPIO_TypeDef *GPIOx, uint8_t pin, pin_pupd pupd)
{
    uint32_t temp = GPIOx->PUPDR;
    temp &= ~(3U << (pin << 1));
    temp |= (pupd & 3U) << (pin << 1);
    GPIOx->PUPDR = temp;
}

/* 使能 SYSCFG 时钟（通常在初始化时做一次） */
static void hal_syscfg_clock_enable(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // RCC_APB2ENR bit 14 使能 SYSCFG
    // 注意：如果 RCC 结构未包含 APB2ENR，需自行添加或全手动写寄存器
}

/* 将 GPIO 端口映射到 EXTI 线 */
static void hal_syscfg_exti_line_config(gpio_port_t port, uint8_t pin) {
    uint32_t shift = (pin % 4) * 4;
    uint32_t reg_idx = pin / 4;
    uint32_t temp = xSYSCFG->EXTICR[reg_idx];
    temp &= ~(0xFUL << shift);
    temp |= (uint32_t)port << shift;
    xSYSCFG->EXTICR[reg_idx] = temp;

    //xSYSCFG->EXTICR[pin >> 2] &= ~(0xFUL << ((pin & 0x03) << 2));  // 清除原有设置，让EXTI0映射到PA0
    //xSYSCFG->EXTICR[pin >> 2] |= port << ((pin & 0x03) << 2);//SYSCFG_EXTICR2_EXTI6_PB;
}

/* 配置 EXTI 触发类型 */
static void hal_exti_set_trigger(uint8_t pin, exti_mode irq_mode) {
    uint32_t bit = 1U << pin;
    switch (irq_mode) {
        case PIN_IRQ_MODE_RISING:
            xEXTI->RTSR |= bit;
            xEXTI->FTSR &= ~bit;
            break;
        case PIN_IRQ_MODE_FALLING:
            xEXTI->FTSR |= bit;
            xEXTI->RTSR &= ~bit;
            break;
        case PIN_IRQ_MODE_BOTH:
            xEXTI->RTSR |= bit;
            xEXTI->FTSR |= bit;
            break;
        default:
            break;
    }
}

/* 使能 EXTI 中断（NVIC） */
static void hal_exti_enable_irq(uint8_t pin, const irq_config *irq_cfg) {
    xEXTI->IMR |= (1U << pin);
    // NVIC 配置取决于 pin，STM32F4 中 EXTI0..4 有独立 IRQ 号，EXTI5_9、EXTI10_15 分别共享
    // 这里仅举例使能 EXTI1 (IRQn=23)
    // 若你的系统已封装 NVIC 接口，直接调用。下面为最简寄存器形式（Cortex-M4）：
    //NVIC_EnableIRQ(EXTI1_IRQn);  // 需要根据 pin 决定 IRQn
    intc_irq_num new_irq_num;
    if (pin < 5) {
        new_irq_num = EXTI0_IRQ + pin;
    } else if (pin < 10) {
        new_irq_num = EXTI5_9_IRQ;
    } else {
        new_irq_num = EXTI10_15_IRQ;
    }

    if (gloable_intc->fun->register_handler(gloable_intc, new_irq_num, irq_cfg->handler, irq_cfg->arg)) {
        //self->semaphore = sem;
        gloable_intc->fun->attach_semaphore(gloable_intc, new_irq_num, irq_cfg->semaphore);
        gloable_intc->fun->set_priority(gloable_intc, new_irq_num, irq_cfg->priority);
    } else {
        LOG_ERROR("pinmux", "attach irq %d error", new_irq_num);
    }

}

/* 将配置写入硬件 */
static void apply_config(const pin_config_t *cfg, const irq_config *irq_cfg)
{
    xGPIO_TypeDef *gpio = GPIOx[cfg->port];
    hal_gpio_clock_enable(cfg->port);
    hal_gpio_set_mode(gpio, cfg->pin, cfg->mode);
    hal_gpio_set_otype(gpio, cfg->pin, cfg->otype);
    hal_gpio_set_ospeed(gpio, cfg->pin, cfg->ospeed);
    hal_gpio_set_pupd(gpio, cfg->pin, cfg->pupd);
    if (cfg->mode == PIN_MODE_AF) {
        hal_gpio_set_af(gpio, cfg->pin, cfg->af);
    }
    /* 中断配置 */
    if (cfg->mode == PIN_MODE_INPUT && cfg->irq_mode != PIN_IRQ_MODE_NONE) {
        hal_syscfg_exti_line_config(cfg->port, cfg->pin);
        hal_exti_set_trigger(cfg->pin, cfg->irq_mode);
        hal_exti_enable_irq(cfg->pin, irq_cfg);
    }
}

/* ---------- 核心接口 ---------- */
void pinmux_init(void)
{
    for (int port = 0; port < PORT_MAX; port++) {
        for (int pin = 0; pin < 16; pin++) {
            pin_states[port][pin].allocated = false;
        }
        clock_enabled[port] = false;
    }
    hal_syscfg_clock_enable();
}

int pinmux_request(const pin_config_t *cfg, const irq_config *irq_cfg)
{
    if (cfg->port >= PORT_MAX || cfg->pin > 15) {
        LOG_DEBUG("pinmux", "cfg->port %d cfg->pin %d > 15", cfg->port, cfg->pin);
        return PINMUX_ERROR;
    }

    pin_state_t *state = &pin_states[cfg->port][cfg->pin];

    if (state->allocated) {
        /* 已经分配，检查配置是否一致 */
        const pin_config_t *old = &state->config;
        if (old->mode   != cfg->mode   ||
            old->otype  != cfg->otype  ||
            old->ospeed != cfg->ospeed ||
            old->pupd   != cfg->pupd   ||
            (cfg->mode == PIN_MODE_AF && old->af != cfg->af))
        {
            LOG_DEBUG("pinmux", "cfg->port %d cfg->pin %d allocated", cfg->port, cfg->pin);
            return PINMUX_ERROR; /* 冲突 */
        }
        return PINMUX_SUCCESS;     /* 同一配置，允许重复请求 */
    }

    /* 分配新引脚 */
    apply_config(cfg, irq_cfg);
    state->allocated = true;
    state->config = *cfg;
    return PINMUX_SUCCESS;
}

int pinmux_release(gpio_port_t port, uint8_t pin)
{
    if (port >= PORT_MAX || pin > 15)
        return PINMUX_ERROR;

    pin_states[port][pin].allocated = false;
    /* 注意：不改变硬件配置，例如可将引脚设为输入浮空，视需求实现 */
    return PINMUX_SUCCESS;
}

int pinmux_request_group(const pin_config_t *cfgs, int count, const irq_config *irq_cfg)
{
    for (int i = 0; i < count; i++) {
        if (pinmux_request(&cfgs[i], irq_cfg) != PINMUX_SUCCESS)
            return PINMUX_ERROR; /* 某一项失败，实际应回滚，这里简化处理 */
    }
    return PINMUX_SUCCESS;
}
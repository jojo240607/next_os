#include "pinmux.h"
#include "device.h"
#include "../../log/log.h"
#include "../hal/hal_exti.h"
#include "rcc.h"


/* ---------- 引脚状态管理 ---------- */
typedef struct {
    bool         allocated;
    pin_config_t config;
} pin_state_t;

static pin_state_t pin_states[PORT_MAX][PIN_MAX];

/* 将配置写入硬件 */
static void apply_config(const pin_config_t *cfg)
{
    gpio_port_t port;
    gpio_pin_t  pin;
    if (cfg->mode == PIN_MODE_AF) {
        port = AF_REQ_GET_PORT(cfg->af);
        pin = AF_REQ_GET_PIN(cfg->af);
    } else {
        port = cfg->port;
        pin = cfg->pin;
    }

    xGPIO_TypeDef *gpio = GPIOx[port];

    hal_gpio_clock_enable(port);
    hal_gpio_set_mode(gpio, pin, cfg->mode);
    hal_gpio_set_otype(gpio, pin, cfg->otype);
    hal_gpio_set_ospeed(gpio, pin, cfg->ospeed);
    hal_gpio_set_pupd(gpio, pin, cfg->pupd);

    if (cfg->mode == PIN_MODE_AF) {
        pin_af_mode af_mode = AF_REQ_GET_MODE(cfg->af);
        LOG_DEBUG("pinmux", "use gpio af port %d pin %d af mode %d", port, pin, af_mode);
        hal_gpio_set_af(gpio, pin, af_mode);
    } else if (cfg->mode == PIN_MODE_INPUT && cfg->irq_mode != PIN_IRQ_MODE_NONE) {
        LOG_DEBUG("pinmux", "use gpio exti port %d pin %d", port, pin);
        /* 中断配置 */
        hal_syscfg_exti_line_config(cfg->port, cfg->pin);
        hal_exti_set_trigger(cfg->pin, cfg->irq_mode);
        hal_exti_enable_irq(cfg->pin);
    } else {
        LOG_DEBUG("pinmux", "use gpio as normal port %d pin %d mode %d", port, pin, cfg->mode);
    }

}

/* ---------- 核心接口 ---------- */
void pinmux_init(void)
{
    LOG_DEBUG("pinmux", "pinmux init");
    for (int port = 0; port < PORT_MAX; port++) {
        for (int pin = 0; pin < 16; pin++) {
            pin_states[port][pin].allocated = false;
        }
    }
}

int pinmux_request(const pin_config_t *cfg)
{
    gpio_port_t port;
    gpio_pin_t  pin;
    if (cfg->mode == PIN_MODE_AF) {
        port = AF_REQ_GET_PORT(cfg->af);
        pin = AF_REQ_GET_PIN(cfg->af);
    } else {
        port = cfg->port;
        pin = cfg->pin;
    }

    if (port >= PORT_MAX || pin >= PIN_MAX) {
        LOG_DEBUG("pinmux", "cport %d pin %d >= PIN_MAX", port, pin);
        return PINMUX_ERROR;
    }
    pin_state_t *state = &pin_states[port][pin];

    if (state->allocated) {
        /* 已经分配，检查配置是否一致 */
        const pin_config_t *old = &state->config;
        if (old->mode   != cfg->mode   ||
            old->otype  != cfg->otype  ||
            old->ospeed != cfg->ospeed ||
            old->pupd   != cfg->pupd   ||
            (cfg->mode == PIN_MODE_AF && old->af != cfg->af))
        {
            LOG_DEBUG("pinmux", "error: cfg->port %d cfg->pin %d have allocated", port, pin);
            return PINMUX_ERROR; /* 冲突 */
        }
        return PINMUX_SUCCESS;     /* 同一配置，允许重复请求 */
    }

    /* 分配新引脚 */
    apply_config(cfg);
    state->allocated = true;
    state->config = *cfg;
    return PINMUX_SUCCESS;
}

int pinmux_release(gpio_port_t port, uint8_t pin)
{
    if (port >= PORT_MAX || pin >= PIN_MAX)
        return PINMUX_ERROR;

    pin_states[port][pin].allocated = false;
    /* 注意：不改变硬件配置，例如可将引脚设为输入浮空，视需求实现 */
    return PINMUX_SUCCESS;
}

int pinmux_request_group(const pin_config_t *cfgs, int count)
{
    for (int i = 0; i < count; i++) {
        if (pinmux_request(&cfgs[i]) != PINMUX_SUCCESS)
            return PINMUX_ERROR; /* 某一项失败，实际应回滚，这里简化处理 */
    }
    return PINMUX_SUCCESS;
}
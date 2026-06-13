/**
 * I2C 驱动 — 公共部分 (构造、硬件初始化、ISR)
 *
 * dev_read/write/ioctl/dev_init 按模式分散在各模式文件中，
 * init 时一次选择，运行时零 if-else。
 */
#include "i2c.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../hal/hal_i2c.h"
#include "../common/rcc.h"

/* ── 模式文件外部声明 ── */
extern size_t i2c_poll_read(Device*, void*, size_t);
extern void   i2c_poll_write(Device*, const void*, size_t);
extern void   i2c_poll_ioctl(Device*, ioctl_cmd_t, void*);
extern void   i2c_poll_dev_init(Device*);
extern size_t i2c_it_read(Device*, void*, size_t);
extern void   i2c_it_write(Device*, const void*, size_t);
extern void   i2c_it_ioctl(Device*, ioctl_cmd_t, void*);
extern void   i2c_it_dev_init(Device*);
extern size_t i2c_dma_read(Device*, void*, size_t);
extern void   i2c_dma_write(Device*, const void*, size_t);
extern void   i2c_dma_ioctl(Device*, ioctl_cmd_t, void*);
extern void   i2c_dma_dev_init(Device*);

static void i2c_destroy(I2c* self);
static void i2c_hw_init(I2c *self);

static const I2cFun i2c_fun = { .destroy = i2c_destroy };

/* ════ 构造 / 析构 ════ */
I2c* i2c_create(const device_info_t *info) {
    I2c* obj = os_malloc(sizeof(I2c));
    if (obj) { memset(obj, 0, sizeof(I2c)); i2c_init(obj, info); }
    return obj;
}

void i2c_init(I2c* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &i2c_fun;

    const i2c_config_t *conf = (const i2c_config_t *)info->conf;
    if (conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = i2c_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = i2c_dma_write;
        GET_DEVICE_VTABLE(self)->dev_read  = i2c_dma_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = i2c_dma_ioctl;
    } else if (conf->it_enable) {
        GET_DEVICE_VTABLE(self)->dev_init  = i2c_it_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = i2c_it_write;
        GET_DEVICE_VTABLE(self)->dev_read  = i2c_it_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = i2c_it_ioctl;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = i2c_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = i2c_poll_write;
        GET_DEVICE_VTABLE(self)->dev_read  = i2c_poll_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = i2c_poll_ioctl;
    }

    self->i2c_xfer  = NULL;
    self->slave_addr = 0x50;
    self->i2c_sem = semaphore_create(0);

    /* 公共硬件初始化: PinMux + 时钟 + 使能 + listener, 立即执行 */
    i2c_hw_init(self);
}

void i2c_deinit(I2c* self) { device_deinit(GET_DEVICE(self)); }
static void i2c_destroy(I2c* self) {
    if (self) { i2c_deinit(self); os_free(self); }
}

static void default_i2c_listener(Device *self, uint8_t event, void *arg) {
    I2c *i2c = GET_I2C(self);
    switch (event) {
        case I2C_TX_START: case I2C_RX_START:
            i2c->i2c_sem->fun->take(i2c->i2c_sem); break;
        case I2C_TX_DONE: case I2C_RX_DONE: case I2C_TRANS_ERROR:
            i2c->i2c_sem->fun->give(i2c->i2c_sem); break;
        default: break;
    }
}

/* ════ 公共硬件初始化 ════ */
static void i2c_hw_init(I2c *self) {
    Device *dev = GET_DEVICE(self);
    const i2c_config_t *conf = dev->info->conf;
    if (conf->id >= I2C_MAX) return;

    const i2c_pins_t *p = &conf->pins;
    pin_config_t pins[] = {
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_OD, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->scl_pin },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_OD, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->sda_pin },
    };
    for (int i = 0; i < 2; i++)
        if (pinmux_request(&pins[i]) != PINMUX_SUCCESS) return;

    hal_i2c_clock_enable(conf->id);
    hal_i2c_reset(conf->id);
    hal_i2c_set_clock(conf->id, conf->clock_speed);
    hal_i2c_set_addr(conf->id, conf->addr_mode, conf->own_address);
    hal_i2c_enable(conf->id);

    dev->fun->register_listener(dev, default_i2c_listener);
    LOG_DEBUG("i2c", "i2c%d init ok", conf->id);
}

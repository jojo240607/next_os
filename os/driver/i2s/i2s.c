/**
 * I2S 驱动 — 公共部分
 */
#include "i2s.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../common/rcc.h"

extern void i2s_poll_dev_init(Device*);
extern size_t i2s_poll_read(Device*, void*, size_t);
extern void i2s_poll_write(Device*, const void*, size_t);
extern void i2s_poll_ioctl(Device*, ioctl_cmd_t, void*);
extern void i2s_it_dev_init(Device*);
extern size_t i2s_it_read(Device*, void*, size_t);
extern void i2s_it_write(Device*, const void*, size_t);
extern void i2s_it_ioctl(Device*, ioctl_cmd_t, void*);
extern void i2s_dma_dev_init(Device*);
extern size_t i2s_dma_read(Device*, void*, size_t);
extern void i2s_dma_write(Device*, const void*, size_t);
extern void i2s_dma_ioctl(Device*, ioctl_cmd_t, void*);

static void i2s_hw_init(I2s *self);
static void i2s_destroy(I2s* self);
static const I2sFun i2s_fun = { .destroy = i2s_destroy };

static void default_i2s_listener(Device *self, uint8_t event, void *arg) {
    I2s *i2s = GET_I2S(self);
    (void)arg;
    switch (event) {
        case I2S_TX_START: i2s->i2s_tx_sem->fun->take(i2s->i2s_tx_sem); break;
        case I2S_TX_DONE:  i2s->i2s_tx_sem->fun->give(i2s->i2s_tx_sem); break;
        case I2S_RX_START: i2s->i2s_rx_sem->fun->take(i2s->i2s_rx_sem); break;
        case I2S_RX_DONE:  i2s->i2s_rx_sem->fun->give(i2s->i2s_rx_sem); break;
        default: break;
    }
}

I2s* i2s_create(const device_info_t *info) {
    I2s* obj = (I2s*)os_malloc(sizeof(I2s));
    if (obj) { memset(obj, 0, sizeof(I2s)); i2s_init(obj, info); }
    return obj;
}

void i2s_init(I2s* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &i2s_fun;

    const i2s_config_t *conf = (const i2s_config_t *)info->conf;
    if (conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = i2s_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = i2s_dma_read;
        GET_DEVICE_VTABLE(self)->dev_write = i2s_dma_write;
    } else if (conf->it_enable) {
        GET_DEVICE_VTABLE(self)->dev_init  = i2s_it_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = i2s_it_read;
        GET_DEVICE_VTABLE(self)->dev_write = i2s_it_write;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = i2s_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = i2s_poll_read;
        GET_DEVICE_VTABLE(self)->dev_write = i2s_poll_write;
    }
    GET_DEVICE_VTABLE(self)->dev_ioctl = i2s_poll_ioctl;

    self->i2s_xfer  = NULL;
    self->i2s_tx_sem = semaphore_create(0);
    self->i2s_rx_sem = semaphore_create(0);

    i2s_hw_init(self);
}

void i2s_deinit(I2s* self) { device_deinit(GET_DEVICE(self)); }
static void i2s_destroy(I2s* self) { if (self) { i2s_deinit(self); os_free(self); } }

static void i2s_hw_init(I2s *self) {
    Device *dev = GET_DEVICE(self);
    const i2s_config_t *conf = dev->info->conf;
    if (!conf || conf->id >= I2S_MAX) return;

    hal_i2s_clock_enable(conf->id);

    const i2s_pins_t *p = &conf->pins;
    pin_config_t pins[4] = {
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->sck_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->sd_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->ws_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->mck_pin },
    };
    if (pinmux_request_group(pins, conf->enable_mck ? 4 : 3) != 0) return;

    hal_i2s_config_pll(conf->id, conf->plli2s_n, conf->plli2s_r);
    hal_i2s_config_i2spr(conf->id, conf->i2s_div, conf->odd_factor, conf->enable_mck);

    bool pcm_sync = (conf->standard == xI2S_STANDARD_PCM_LONG);
    hal_i2s_config_i2scfgr(conf->id, conf->mode, conf->standard,
                           conf->data_format, conf->clock_polarity, pcm_sync);

    dev->fun->register_listener(dev, default_i2s_listener);
    LOG_DEBUG("i2s", "i2s%d hw init ok", conf->id);
}

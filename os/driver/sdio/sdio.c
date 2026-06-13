/**
 * SDIO 驱动 — 公共部分
 */
#include "sdio.h"
#include "../../common/linear_pool.h"
#include "../common/rcc.h"

extern void sdio_poll_dev_init(Device*);
extern size_t sdio_poll_read(Device*, void*, size_t);
extern void   sdio_poll_write(Device*, const void*, size_t);
extern void   sdio_poll_ioctl(Device*, ioctl_cmd_t, void*);
extern void   sdio_it_dev_init(Device*);
extern size_t sdio_it_read(Device*, void*, size_t);
extern void   sdio_it_write(Device*, const void*, size_t);
extern void   sdio_it_ioctl(Device*, ioctl_cmd_t, void*);
extern void   sdio_dma_dev_init(Device*);
extern size_t sdio_dma_read(Device*, void*, size_t);
extern void   sdio_dma_write(Device*, const void*, size_t);
extern void   sdio_dma_ioctl(Device*, ioctl_cmd_t, void*);

static void sdio_destroy(Sdio* self);
static const SdioFun sdio_fun = { .destroy = sdio_destroy };

static void default_sdio_listener(Device *self, uint8_t event, void *arg) {
    Sdio *sdio = GET_SDIO(self);
    (void)arg;
    switch (event) {
        case SDIO_XFER_START: sdio->sdio_sem->fun->take(sdio->sdio_sem); break;
        case SDIO_CMD_DONE:
        case SDIO_DATA_DONE:
        case SDIO_XFER_ERROR: sdio->sdio_sem->fun->give(sdio->sdio_sem); break;
        default: break;
    }
}

Sdio* sdio_create(const device_info_t *info) {
    Sdio* obj = (Sdio*)os_malloc(sizeof(Sdio));
    if (obj) { memset(obj, 0, sizeof(Sdio)); sdio_init(obj, info); }
    return obj;
}

void sdio_init(Sdio* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &sdio_fun;

    const sdio_config_t *conf = (const sdio_config_t *)info->conf;
    if (conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = sdio_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = sdio_dma_read;
        GET_DEVICE_VTABLE(self)->dev_write = sdio_dma_write;
        GET_DEVICE_VTABLE(self)->dev_ioctl = sdio_dma_ioctl;
    } else if (conf->it_enable) {
        GET_DEVICE_VTABLE(self)->dev_init  = sdio_it_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = sdio_it_read;
        GET_DEVICE_VTABLE(self)->dev_write = sdio_it_write;
        GET_DEVICE_VTABLE(self)->dev_ioctl = sdio_it_ioctl;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = sdio_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = sdio_poll_read;
        GET_DEVICE_VTABLE(self)->dev_write = sdio_poll_write;
        GET_DEVICE_VTABLE(self)->dev_ioctl = sdio_poll_ioctl;
    }

    self->sdio_sem    = semaphore_create(0);
    self->block_addr  = 0;
    self->block_size  = 512;
    self->kwork       = NULL;

    if (!conf) return;
    hal_sdio_clock_enable();

    const sdio_pins_t *p = &conf->pins;
    const pin_config_t pin_cfgs[] = {
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->clk_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = p->cmd_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = p->d0_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = p->d1_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = p->d2_pin },
        { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = p->d3_pin },
    };
    if (pinmux_request_group(pin_cfgs, 6) != 0) return;

    hal_sdio_power_on();
    uint32_t clkcr = hal_sdio_build_clkcr(conf->clock_div, conf->bus_width,
        conf->clock_edge, conf->power_save, conf->hw_flow_control);
    hal_sdio_config_clock(clkcr);

    GET_DEVICE(self)->fun->register_listener(GET_DEVICE(self), default_sdio_listener);
}

void sdio_deinit(Sdio* self) { device_deinit(GET_DEVICE(self)); }
static void sdio_destroy(Sdio* self) { if (self) { sdio_deinit(self); os_free(self); } }





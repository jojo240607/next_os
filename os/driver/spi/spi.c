/**
 * SPI 驱动 — 公共部分 (构造、硬件初始化)
 *
 * dev_read/write/ioctl/dev_init 按模式分散在各模式文件中，
 * init 时一次选择，运行时零 if-else。
 */
#include "spi.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../hal/hal_spi.h"

/* ── 模式文件外部声明 ── */
extern size_t spi_poll_read(Device*, void*, size_t);
extern void   spi_poll_write(Device*, const void*, size_t);
extern void   spi_poll_ioctl(Device*, ioctl_cmd_t, void*);
extern void   spi_poll_dev_init(Device*);
extern size_t spi_it_read(Device*, void*, size_t);
extern void   spi_it_write(Device*, const void*, size_t);
extern void   spi_it_ioctl(Device*, ioctl_cmd_t, void*);
extern void   spi_it_dev_init(Device*);
extern size_t spi_dma_read(Device*, void*, size_t);
extern void   spi_dma_write(Device*, const void*, size_t);
extern void   spi_dma_ioctl(Device*, ioctl_cmd_t, void*);
extern void   spi_dma_dev_init(Device*);

static void spi_hw_init(Spi *self);
static void default_spi_listener(Device *self, uint8_t event, void *arg);
static void spi_destroy(Spi* self);
static const SpiFun spi_fun = { .destroy = spi_destroy };

/* ════ 构造 / 析构 ════ */
Spi* spi_create(const device_info_t *info)
{
    Spi* obj = (Spi*)os_malloc(sizeof(Spi));
    if (obj) { memset(obj, 0, sizeof(Spi)); spi_init(obj, info); }
    return obj;
}

void spi_init(Spi* self, const device_info_t *info)
{
    device_init(&self->base, info);
    self->fun = &spi_fun;

    const spi_config_t *conf = (const spi_config_t *)info->conf;
    if (conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = spi_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = spi_dma_write;
        GET_DEVICE_VTABLE(self)->dev_read  = spi_dma_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = spi_dma_ioctl;
    } else if (conf->it_enable) {
        GET_DEVICE_VTABLE(self)->dev_init  = spi_it_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = spi_it_write;
        GET_DEVICE_VTABLE(self)->dev_read  = spi_it_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = spi_it_ioctl;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = spi_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = spi_poll_write;
        GET_DEVICE_VTABLE(self)->dev_read  = spi_poll_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = spi_poll_ioctl;
    }

    self->spi_xfer = NULL;
    self->cs_pin   = NULL;
    self->spi_sem  = semaphore_create(0);

    /* 公共硬件初始化: PinMux + 时钟 + 使能, 立即执行 */
    spi_hw_init(self);
}

void spi_deinit(Spi* self) { device_deinit(GET_DEVICE(self)); }
static void spi_destroy(Spi* self) {
    if (self) { spi_deinit(self); os_free(self); }
}

/* ════ 公共硬件初始化 ════ */
static void spi_hw_init(Spi *self) {
    Device *dev = GET_DEVICE(self);
    const spi_config_t *conf = dev->info->conf;
    if (conf->id >= SPI_MAX) return;

    const spi_pins_t *p = &conf->pins;
    pin_config_t pins[] = {
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->sck_pin },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->mosi_pin },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->miso_pin },
        { .port = p->cs_pin.port, .pin = p->cs_pin.pin,
          .mode = (conf->nss_mode == SPI_NSS_HARD) ? PIN_MODE_AF : PIN_MODE_OUTPUT,
          .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP,
          .af = (conf->nss_mode == SPI_NSS_HARD) ? p->nss_pin : 0 },
    };
    for (int i = 0; i < 4; i++)
        if (pinmux_request(&pins[i]) != PINMUX_SUCCESS) return;

    if (conf->nss_mode == SPI_NSS_SOFT) {
        self->cs_pin = &p->cs_pin;
        gpio_set(self->cs_pin);
    }

    hal_spi_clock_enable(conf->id);
    hal_spi_init(conf->id, conf->mode, conf->frame_format,
                 conf->baudrate_div, conf->master, conf->first_bit,
                 conf->nss_mode);
    hal_spi_disable_it(conf->id);

    dev->fun->register_listener(dev, default_spi_listener);
    LOG_DEBUG("spi", "spi%d hw init ok", conf->id);
}

/* ════ 内部 listener — 所有 SPI 模式共用 ════ */
static void default_spi_listener(Device *self, uint8_t event, void *arg) {
    Spi *spi = GET_SPI(self);
    (void)arg;
    switch (event) {
        case SPI_XFER_START:
            spi->spi_sem->fun->take(spi->spi_sem); break;
        case SPI_XFER_DONE:
            spi->spi_sem->fun->give(spi->spi_sem); break;
        default: break;
    }
}

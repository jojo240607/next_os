/**
 * ADC 驱动 — 公共部分 (构造、硬件初始化)
 */
#include "adc.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../hal/hal_adc.h"

/* ── 模式文件外部声明 ── */
extern void   adc_poll_dev_init(Device*);
extern size_t adc_poll_read(Device*, void*, size_t);
extern void   adc_dma_dev_init(Device*);
extern size_t adc_dma_read(Device*, void*, size_t);

void adc_hw_init(Adc *self);
static void adc_destroy(Adc* self);
static const AdcFun adc_fun = { .destroy = adc_destroy, .start_dma = NULL };

/* ════ 构造 / 析构 ════ */
Adc* adc_create(const device_info_t *info) {
    Adc* obj = (Adc*)os_malloc(sizeof(Adc));
    if (obj) { memset(obj, 0, sizeof(Adc)); adc_init(obj, info); }
    return obj;
}

void adc_init(Adc* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &adc_fun;

    const adc_config_t *conf = (const adc_config_t *)info->conf;
    if (conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = adc_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = adc_dma_read;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = adc_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = adc_poll_read;
    }
    GET_DEVICE_VTABLE(self)->dev_write = NULL;
    GET_DEVICE_VTABLE(self)->dev_ioctl = NULL;

    self->adc_sem = semaphore_create(0);

    adc_hw_init(self);
}

void adc_deinit(Adc* self) { device_deinit(GET_DEVICE(self)); }
static void adc_destroy(Adc* self) {
    if (self) { adc_deinit(self); os_free(self); }
}

/* ════ 内部 listener ════ */
static void default_adc_listener(Device *self, uint8_t event, void *arg) {
    Adc *adc = GET_ADC(self);
    (void)arg;
    switch (event) {
        case ADC_XFER_START: adc->adc_sem->fun->take(adc->adc_sem); break;
        case ADC_XFER_DONE:  adc->adc_sem->fun->give(adc->adc_sem); break;
        default: break;
    }
}

/* ════ 公共硬件初始化 ════ */
void adc_hw_init(Adc *self) {
    Device *dev = GET_DEVICE(self);
    const adc_config_t *conf = dev->info->conf;
    if (conf->id >= ADC_MAX) return;

    /* 1. PinMux 模拟模式 */
    for (int i = 0; i < conf->num_channels; i++) {
        pin_config_t pin_cfg = {
            .port   = conf->channels[i]->port,
            .pin    = conf->channels[i]->pin,
            .mode   = PIN_MODE_ANALOG,
            .otype  = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_LOW,
            .pupd   = PIN_PUPD_NONE, .irq_mode = PIN_IRQ_MODE_NONE,
        };
        if (pinmux_request(&pin_cfg) != PINMUX_SUCCESS) return;
    }

    /* 2. 时钟 */
    hal_adc_clock_enable(conf->id);

    /* 3. 分辨率 + 对齐 */
    hal_adc_set_resolution_align(conf->id, conf->resolution, conf->align);

    /* 4. 采样时间 */
    for (int i = 0; i < conf->num_channels; i++)
        hal_adc_set_sample_time(conf->id, conf->channels[i]->channel,
                                conf->channels[i]->sample_time);

    /* 5. 规则通道序列 */
    uint8_t ch_list[16];
    for (int i = 0; i < conf->num_channels; i++)
        ch_list[i] = conf->channels[i]->channel;
    hal_adc_set_regular_sequence(conf->id, conf->num_channels, ch_list);

    /* 6. 使能 + 校准 */
    hal_adc_enable(conf->id);
    hal_adc_start_calibration(conf->id);

    dev->fun->register_listener(dev, default_adc_listener);
    LOG_DEBUG("adc", "adc%d hw init ok", conf->id);
}

/**
 * ADC DMA 模式
 */
#include "adc.h"
#include "../hal/hal_adc.h"
#include "../common/dma.h"

/* ── DMA 中断处理 ── */
static bool adc_dma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    const adc_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg);
    dev->fun->trigger_event(dev, ADC_XFER_DONE, dev->arg);
    return true;
}

/* ── dev_init ── */
void adc_dma_dev_init(Device *self) {
    const adc_config_t *conf = self->info->conf;

    dma_stream_request(conf->dma_cfg);
    if (conf->dma_cfg->it_enable) {
        self->irq_conf->handler = adc_dma_irq_handler;
        self->irq_conf->arg = self;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
            dma_get_irqnum(conf->dma_cfg));
        self->fun->config_irq(self, self->irq_conf);
    }

    hal_adc_enable_dma(conf->id, conf->continuous);
}

/* ── dev_read (DMA) ── */
size_t adc_dma_read(Device *self, void *buf, size_t count) {
    Adc *adc = GET_ADC(self);
    const adc_config_t *conf = self->info->conf;
    if (conf->id >= ADC_MAX) return 0;

    uint32_t per_addr = hal_adc_get_dr_addr(conf->id);
    dma_start_transfer(conf->dma_cfg, per_addr, (uint32_t)buf, (uint16_t)(count / 2));
    self->fun->trigger_event(self, ADC_XFER_START, self->arg);
    return count;
}

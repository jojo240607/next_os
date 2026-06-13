/**
 * ADC 轮询模式
 */
#include "adc.h"
#include "../hal/hal_adc.h"

void adc_poll_dev_init(Device *self) { (void)self; }

size_t adc_poll_read(Device *self, void *buf, size_t count) {
    if (count < sizeof(uint16_t)) return 0;
    const adc_config_t *conf = self->info->conf;
    if (conf->id >= ADC_MAX) return 0;

    hal_adc_start_conversion(conf->id);
    hal_adc_wait_eoc(conf->id);
    *(uint16_t *)buf = hal_adc_read_dr(conf->id);
    return sizeof(uint16_t);
}

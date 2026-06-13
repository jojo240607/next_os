/**
 * I2S 轮询模式
 */
#include "i2s.h"
#include "../hal/hal_i2s.h"

void i2s_poll_dev_init(Device *self) { (void)self; }
void i2s_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

size_t i2s_poll_read(Device *self, void *buf, size_t count) {
    const i2s_config_t *conf = self->info->conf;
    if (count < 2) return 0;
    hal_i2s_receive(conf->id, (uint16_t *)buf, (uint16_t)(count / 2));
    return count;
}

void i2s_poll_write(Device *self, const void *buf, size_t count) {
    const i2s_config_t *conf = self->info->conf;
    if (count < 2) return;
    hal_i2s_transmit(conf->id, (const uint16_t *)buf, (uint16_t)(count / 2));
}

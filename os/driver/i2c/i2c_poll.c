/**
 * I2C 轮询模式
 */
#include "i2c.h"
#include "../hal/hal_i2c.h"

/* ── dev_init ── */
void i2c_poll_dev_init(Device *self) { (void)self; }

/* ── ioctl ── */
void i2c_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    i2c_transfer_args_t *a = (i2c_transfer_args_t *)arg;
    if (!a) return;
    i2c->slave_addr = a->slave_addr;
    switch (cmd) {
    case I2C_IOCTL_TRANSFER_TX:
        if (a->tx_buf && a->tx_len > 0)
            hal_i2c_transmit(conf->id, a->slave_addr, a->tx_buf, a->tx_len);
        break;
    case I2C_IOCTL_TRANSFER_RX:
        if (a->tx_buf && a->tx_len > 0)
            hal_i2c_transmit(conf->id, a->slave_addr, a->tx_buf, a->tx_len);
        if (a->rx_buf && a->rx_len > 0)
            hal_i2c_receive(conf->id, a->slave_addr, a->rx_buf, a->rx_len);
        break;
    default: break;
    }
}

/* ── 读写 ── */
size_t i2c_poll_read(Device *self, void *buf, size_t count) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (conf->id >= I2C_MAX || count == 0) return 0;
    hal_i2c_receive(conf->id, i2c->slave_addr, (uint8_t *)buf, (uint16_t)count);
    return count;
}

void i2c_poll_write(Device *self, const void *buf, size_t count) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (conf->id >= I2C_MAX || count == 0) return;
    hal_i2c_transmit(conf->id, i2c->slave_addr, buf, (uint16_t)count);
}

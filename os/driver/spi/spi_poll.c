/**
 * SPI 轮询模式
 */
#include "spi.h"
#include "../hal/hal_spi.h"

/* ── dev_init ── */
void spi_poll_dev_init(Device *self) { (void)self; }

/* ── ioctl (轮询) ── */
void spi_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    if (cmd == DEVICE_TRANSFER && arg) {
        spi_transfer_args_t *a = (spi_transfer_args_t *)arg;
        const gpio_t *cs = a->cs_pin ? a->cs_pin : spi->cs_pin;
        hal_spi_transfer(conf->id, cs, a->tx_buf, a->rx_buf, a->len);
    }
}

/* ── 读写 ── */
size_t spi_poll_read(Device *self, void *buf, size_t count) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    if (conf->id >= SPI_MAX || count == 0) return 0;
    hal_spi_receive(conf->id, spi->cs_pin, buf, count);
    return count;
}

void spi_poll_write(Device *self, const void *buf, size_t count) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    if (conf->id >= SPI_MAX || count == 0) return;
    hal_spi_transmit(conf->id, spi->cs_pin, buf, count);
}

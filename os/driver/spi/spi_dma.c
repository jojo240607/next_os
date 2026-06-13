/**
 * SPI DMA 模式
 */
#include "spi.h"
#include "../hal/hal_spi.h"
#include "../common/dma.h"
#include "../../common/linear_pool.h"

/* ── DMA 中断处理 ── */
static bool spi_txdma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    Spi *spi = GET_SPI(dev);
    const spi_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    if (spi->spi_xfer && spi->spi_xfer->active) {
        spi->spi_xfer->active = false;
        if (spi->spi_xfer->cs_pin) gpio_set(spi->spi_xfer->cs_pin);
        else                        hal_spi_disable(conf->id);
        dev->fun->trigger_event(dev, SPI_XFER_DONE, dev->arg);
    }
    return true;
}

static bool spi_rxdma_irq_handler(nvic_irq_t *irq_conf) {
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    dma_clear_flag(conf->dma_cfg->rx_dma);
    return true;
}

/* ── dev_init ── */
void spi_dma_dev_init(Device *self) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;

    if (!spi->spi_xfer) {
        spi->spi_xfer = os_malloc(sizeof(spi_xfer_t));
        memset(spi->spi_xfer, 0, sizeof(spi_xfer_t));
    }

    if (conf->dma_cfg->tx_dma) {
        if (conf->dma_cfg->tx_dma->it_enable) {
            self->irq_conf->handler = spi_txdma_irq_handler;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(
                self->irq_conf->irq_list,
                dma_get_irqnum(conf->dma_cfg->tx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        dma_stream_request(conf->dma_cfg->tx_dma);
    }
    if (conf->dma_cfg->rx_dma) {
        if (conf->dma_cfg->rx_dma->it_enable) {
            self->irq_conf->handler = spi_rxdma_irq_handler;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(
                self->irq_conf->irq_list,
                dma_get_irqnum(conf->dma_cfg->rx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        dma_stream_request(conf->dma_cfg->rx_dma);
    }
    hal_spi_dma_init(conf->id, conf->dma_cfg->tx_dma, conf->dma_cfg->rx_dma);
}

/* ── ioctl (DMA) ── */
void spi_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    if (cmd == DEVICE_TRANSFER && arg) {
        spi_transfer_args_t *a = (spi_transfer_args_t *)arg;
        const gpio_t *cs = a->cs_pin ? a->cs_pin : spi->cs_pin;

        if (!x || dma_is_busy(conf->dma_cfg->tx_dma)) return;
        if (cs) gpio_reset(cs);
        else    hal_spi_enable(conf->id);

        x->cs_pin = cs;
        x->active = true;
        hal_spi_send_dma(conf->id, conf->dma_cfg->tx_dma, a->tx_buf, a->len);
        hal_spi_recv_dma(conf->id, conf->dma_cfg->rx_dma, a->rx_buf, a->len);
        self->fun->trigger_event(self, SPI_XFER_START, self->arg);
    }
}

/* ── 读写 ── */
size_t spi_dma_read(Device *self, void *buf, size_t count) {
    (void)self; (void)buf; (void)count;
    return -1; /* DMA 读走 TRANSFER, 不走 dev_read */
}

void spi_dma_write(Device *self, const void *buf, size_t count) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    spi_xfer_t *x = spi->spi_xfer;

    if (!x || dma_is_busy(conf->dma_cfg->tx_dma)) return;
    if (spi->cs_pin) gpio_reset(spi->cs_pin);
    x->cs_pin = spi->cs_pin;
    x->active = true;
    hal_spi_send_dma(conf->id, conf->dma_cfg->tx_dma, buf, count);
    self->fun->trigger_event(self, SPI_XFER_START, self->arg);
}

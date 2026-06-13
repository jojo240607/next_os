/**
 * I2C DMA 模式
 */
#include "i2c.h"
#include "../hal/hal_i2c.h"
#include "../common/dma.h"


static bool i2c_txdma_irq_handler_impl(nvic_irq_t *irq_conf)
{
    Device *dev = (Device *)irq_conf->arg;
    const i2c_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    dev->fun->trigger_event(dev, I2C_TX_DONE, dev->arg);
    return true;
}

static bool i2c_rxdma_irq_handler_impl(nvic_irq_t *irq_conf)
{
    Device *dev = (Device *)irq_conf->arg;
    const i2c_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg->rx_dma);
    dev->fun->trigger_event(dev, I2C_RX_DONE, dev->arg);
    return true;
}

/* ── dev_init ── */
void i2c_dma_dev_init(Device *self) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (!i2c->i2c_xfer) {
        i2c->i2c_xfer = os_malloc(sizeof(i2c_xfer_state_t));
        memset(i2c->i2c_xfer, 0, sizeof(i2c_xfer_state_t));
    }
    if (conf->dma_cfg->tx_dma) {
        if (conf->dma_cfg->tx_dma->it_enable) {
            self->irq_conf->handler = i2c_txdma_irq_handler_impl;
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
            self->irq_conf->handler = i2c_rxdma_irq_handler_impl;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(
                    self->irq_conf->irq_list,
                    dma_get_irqnum(conf->dma_cfg->rx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        dma_stream_request(conf->dma_cfg->rx_dma);
    }
    hal_i2c_dma_init(conf->id, conf->dma_cfg->tx_dma, conf->dma_cfg->rx_dma);
}

/* ── ioctl (DMA) ── */
void i2c_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    I2c *i2c = GET_I2C(self);
    i2c_transfer_args_t *a = (i2c_transfer_args_t *)arg;
    if (!a) return;
    i2c->slave_addr = a->slave_addr;
    switch (cmd) {
    case I2C_IOCTL_TRANSFER_TX:
        if (a->tx_buf && a->tx_len > 0)
            //hal_i2c_dma_write(self, a->tx_buf, a->tx_len);
        break;
    case I2C_IOCTL_TRANSFER_RX:
        break;
    default: break;
    }
}

/* ── 读写 ── */
size_t i2c_dma_read(Device *self, void *buf, size_t count) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (conf->id >= I2C_MAX || count == 0) return 0;
    i2c_transmit_dma(conf->id, conf->dma_cfg->rx_dma,
                     i2c->slave_addr, NULL, (uint16_t)count);
    self->fun->trigger_event(self, I2C_RX_START, self->arg);
    return count;
}

void i2c_dma_write(Device *self, const void *buf, size_t count) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (conf->id >= I2C_MAX) return;
    i2c_transmit_dma(conf->id, conf->dma_cfg->tx_dma,
                     i2c->slave_addr, buf, (uint16_t)count);
    self->fun->trigger_event(self, I2C_TX_START, self->arg);
}

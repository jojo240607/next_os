/**
 * I2S DMA 模式
 */
#include "i2s.h"
#include "../hal/hal_i2s.h"
#include "../common/dma.h"
#include "../../common/linear_pool.h"

static bool i2s_txdma_irq_handler(nvic_irq_t *irq_conf);
static bool i2s_rxdma_irq_handler(nvic_irq_t *irq_conf);

/* ── DMA ISR ── */
static bool i2s_txdma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    const i2s_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    dev->fun->trigger_event(dev, I2S_TX_DONE, dev->arg);
    return true;
}

static bool i2s_rxdma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    const i2s_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg->rx_dma);
    dev->fun->trigger_event(dev, I2S_RX_DONE, dev->arg);
    return true;
}

/* ── dev_init ── */
void i2s_dma_dev_init(Device *self) {
    I2s *i2s = GET_I2S(self);
    const i2s_config_t *conf = self->info->conf;

    if (!i2s->i2s_xfer) {
        i2s->i2s_xfer = os_malloc(sizeof(i2s_xfer_t));
        memset(i2s->i2s_xfer, 0, sizeof(i2s_xfer_t));
    }

    if (conf->dma_cfg->tx_dma) {
        dma_stream_request(conf->dma_cfg->tx_dma);
        if (conf->dma_cfg->tx_dma->it_enable) {
            self->irq_conf->handler = i2s_txdma_irq_handler;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
                dma_get_irqnum(conf->dma_cfg->tx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        hal_i2s_enable_txdma(conf->id, true);
    }
    if (conf->dma_cfg->rx_dma) {
        dma_stream_request(conf->dma_cfg->rx_dma);
        if (conf->dma_cfg->rx_dma->it_enable) {
            self->irq_conf->handler = i2s_rxdma_irq_handler;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
                dma_get_irqnum(conf->dma_cfg->rx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        hal_i2s_enable_rxdma(conf->id, true);
    }
    hal_i2s_start(conf->id);
}

/* ── vtable ── */
size_t i2s_dma_read(Device *self, void *buf, size_t count) {
    I2s *i2s = GET_I2S(self);
    const i2s_config_t *conf = self->info->conf;
    if (count < 2) return 0;
    hal_i2s_receive_dma(conf->id, conf->dma_cfg->rx_dma, (uint16_t *)buf, (uint16_t)(count / 2));
    self->fun->trigger_event(self, I2S_RX_START, self->arg);
    return count;
}

void i2s_dma_write(Device *self, const void *buf, size_t count) {
    I2s *i2s = GET_I2S(self);
    const i2s_config_t *conf = self->info->conf;
    if (count < 2) return;
    hal_i2s_transmit_dma(conf->id, conf->dma_cfg->tx_dma, (const uint16_t *)buf, (uint16_t)(count / 2));
    self->fun->trigger_event(self, I2S_TX_START, self->arg);
}

void i2s_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

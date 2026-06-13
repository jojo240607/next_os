/**
 * I2S 中断模式
 */
#include "i2s.h"
#include "../hal/hal_i2s.h"
#include "../../common/linear_pool.h"

static bool i2s_irq_handler(nvic_irq_t *irq_conf);

/* ── dev_init ── */
void i2s_it_dev_init(Device *self) {
    I2s *i2s = GET_I2S(self);
    const i2s_config_t *conf = self->info->conf;

    if (!i2s->i2s_xfer) {
        i2s->i2s_xfer = os_malloc(sizeof(i2s_xfer_t));
        memset(i2s->i2s_xfer, 0, sizeof(i2s_xfer_t));
    }
    hal_i2s_config_it(conf->id, conf->it_enable);
    hal_i2s_start(conf->id);

    self->irq_conf->handler = i2s_irq_handler;
    self->irq_conf->arg = self;
    self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, SPI2_IRQ + conf->id);
    self->fun->config_irq(self, self->irq_conf);
}

/* ── ISR ── */
static bool i2s_irq_handler(nvic_irq_t *irq_conf) {
    I2s *i2s = (I2s *)irq_conf->arg;
    Device *dev = GET_DEVICE(i2s);
    const i2s_config_t *conf = dev->info->conf;
    i2s_xfer_t *x = i2s->i2s_xfer;
    if (!x) return true;

    xI2S_TypeDef *i2s_ctrl = I2Sx[conf->id];
    uint16_t sr = i2s_ctrl->SR;

    if (sr & (1 << 6)) {  // OVR
        volatile uint16_t dummy = i2s_ctrl->DR; (void)dummy;
        i2s_ctrl->I2SCFGR &= ~xI2S_I2SCFGR_I2SE;
        i2s_ctrl->I2SCFGR |= xI2S_I2SCFGR_I2SE;
        x->active = false;
        i2s_ctrl->CR2 &= ~(xI2S_CR2_TXEIE | xI2S_CR2_RXNEIE);
        dev->fun->trigger_event(dev, I2S_XFER_ERROR, dev->arg);
        return true;
    }

    if ((sr & xI2S_SR_RXNE) && (i2s_ctrl->CR2 & xI2S_CR2_RXNEIE)) {
        uint16_t data = (uint16_t)i2s_ctrl->DR;
        if (x->rx_buf && x->rx_index < x->total_len)
            x->rx_buf[x->rx_index] = data;
        x->rx_index++;
    }

    if ((sr & xI2S_SR_TXE) && (i2s_ctrl->CR2 & xI2S_CR2_TXEIE)) {
        if (x->tx_index < x->total_len) {
            i2s_ctrl->DR = x->tx_buf ? x->tx_buf[x->tx_index] : 0x0000;
            x->tx_index++;
        } else {
            i2s_ctrl->CR2 &= ~xI2S_CR2_TXEIE;
            if (x->tx_buf && !x->rx_buf)
                dev->fun->trigger_event(dev, I2S_TX_DONE, dev->arg);
        }
    }

    bool tx_done = (!x->tx_buf) || (x->tx_index >= x->total_len);
    bool rx_done = (!x->rx_buf) || (x->rx_index >= x->total_len);
    if (tx_done && rx_done && x->active) {
        x->active = false;
        i2s_ctrl->CR2 &= ~(xI2S_CR2_TXEIE | xI2S_CR2_RXNEIE);
        if (x->rx_buf)
            dev->fun->trigger_event(dev, I2S_RX_DONE, dev->arg);
    }
    return true;
}

/* ── vtable ── */
size_t i2s_it_read(Device *self, void *buf, size_t count) {
    I2s *i2s = GET_I2S(self);
    const i2s_config_t *conf = self->info->conf;
    if (count < 2) return 0;

    /* hal_i2s_receive_it 内部管理 xfer + sem_take, 这里直接调 */
    hal_i2s_receive_it(conf->id, i2s->i2s_xfer, (uint16_t *)buf, (uint16_t)(count / 2));
    self->fun->trigger_event(self, I2S_RX_START, self->arg);
    return count;
}

void i2s_it_write(Device *self, const void *buf, size_t count) {
    I2s *i2s = GET_I2S(self);
    const i2s_config_t *conf = self->info->conf;
    if (count < 2) return;

    hal_i2s_transmit_it(conf->id, i2s->i2s_xfer, (const uint16_t *)buf, (uint16_t)(count / 2));
    self->fun->trigger_event(self, I2S_TX_START, self->arg);
}

void i2s_it_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

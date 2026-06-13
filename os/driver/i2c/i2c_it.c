/**
 * I2C 中断模式
 */
#include "i2c.h"
#include "../hal/hal_i2c.h"
#include "../../log/log.h"

/* ════ 中断处理 ════ */
static bool i2c_ev_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = irq_conf->arg;
    const i2c_config_t *conf = dev->info->conf;
    i2c_xfer_state_t *s = GET_I2C(dev)->i2c_xfer;
    if (!s || !s->active) return true;

    uint16_t sr1 = hal_i2c_get_it_event(conf->id);
    if (sr1 & I2C_FLG_SB) {
        *hal_i2c_addr(conf->id) = (GET_I2C(dev)->slave_addr << 1) | s->direction;
        hal_i2c_set_it_event(conf->id, xI2C_IE_ITBUFEN); return true;
    }
    if (sr1 & I2C_FLG_ADDR) {
        hal_i2c_clear_addr_flag(conf->id);
        if (s->direction == I2C_RX && s->rx_total_len == 1)
            hal_i2c_close_ack(conf->id);
        return true;
    }
    if (sr1 & I2C_FLG_TXE) {
        if (s->direction == I2C_TX && s->index < s->tx_total_len)
            *hal_i2c_addr(conf->id) = s->tx_buf[s->index++];
        else if (s->direction == I2C_TX)
            hal_i2c_clear_it_event(conf->id, xI2C_IE_ITBUFEN);
    }
    if (sr1 & I2C_FLG_RXNE) {
        if (s->direction == I2C_RX && s->index < s->rx_total_len) {
            s->rx_buf[s->index++] = *hal_i2c_addr(conf->id);
            if (s->index == s->rx_total_len - 1) hal_i2c_close_ack(conf->id);
            if (s->index >= s->rx_total_len) {
                hal_i2c_clear_it_event(conf->id, xI2C_IE_ITBUFEN | xI2C_IE_ITEVTEN);
                hal_i2c_stop(conf->id);
                s->active = false;
                dev->fun->trigger_event(dev, I2C_RX_DONE, dev->arg);
            }
        }
        return true;
    }
    if (sr1 & I2C_FLG_BTF) {
        if (s->direction == I2C_TX && s->index >= s->tx_total_len) {
            if (s->rx_buf && s->rx_total_len > 0) {
                s->index = 0; s->direction = I2C_RX;
                hal_i2c_start(conf->id);
            } else {
                hal_i2c_stop(conf->id);
                hal_i2c_clear_it_event(conf->id, xI2C_IE_ITEVTEN | xI2C_IE_ITBUFEN);
                s->active = false;
                dev->fun->trigger_event(dev, I2C_TX_DONE, dev->arg);
            }
        }
        return true;
    }
    if (sr1 == 0) return true;
    return true;
}

static bool i2c_er_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = irq_conf->arg;
    i2c_xfer_state_t *s = GET_I2C(dev)->i2c_xfer;
    if (!s || !s->active) return true;
    LOG_ERROR("i2c", "irq error");
    s->active = false;
    dev->fun->trigger_event(dev, I2C_TRANS_ERROR, dev->arg);
    return true;
}

void i2c_it_write(Device *self, const void *buf, size_t count);

/* ── dev_init ── */
void i2c_it_dev_init(Device *self) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (!i2c->i2c_xfer) {
        i2c->i2c_xfer = os_malloc(sizeof(i2c_xfer_state_t));
        memset(i2c->i2c_xfer, 0, sizeof(i2c_xfer_state_t));
    }
    self->irq_conf->arg = self;
    if (conf->it_enable & (xI2C_IT_TXE | xI2C_IT_RXNE)) {
        self->irq_conf->handler = i2c_ev_irq_handler;
        self->irq_conf->irq_list->fun->add_int(
            self->irq_conf->irq_list, I2C1_EV_IRQ + conf->id * 2);
        self->fun->config_irq(self, self->irq_conf);
    }
    if (conf->it_enable & xI2C_IT_ERR) {
        self->irq_conf->handler = i2c_er_irq_handler;
        self->irq_conf->irq_list->fun->add_int(
            self->irq_conf->irq_list, I2C1_ER_IRQ + conf->id * 2);
        self->fun->config_irq(self, self->irq_conf);
    }
}

/* ── ioctl (IT) ── */
void i2c_it_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    i2c_transfer_args_t *a = (i2c_transfer_args_t *)arg;
    if (!a) return;
    i2c->slave_addr = a->slave_addr;
    switch (cmd) {
    case I2C_IOCTL_TRANSFER_TX:
        if (a->tx_buf && a->tx_len > 0)
            i2c_it_write(self, a->tx_buf, a->tx_len);
        break;
    case I2C_IOCTL_TRANSFER_RX: {
        i2c_xfer_state_t *s = i2c->i2c_xfer;
        if (!s || s->active) break;
        s->tx_buf    = a->tx_buf;
        s->tx_total_len = a->tx_len;
        s->rx_buf    = a->rx_buf;
        s->rx_total_len = a->rx_len;
        s->index     = 0;
        s->active    = true;
        s->direction = I2C_TX;
        hal_i2c_transmit_it_start(conf->id);
        self->fun->trigger_event(self, I2C_TX_START, self->arg);
        break;
    }
    default: break;
    }
}

/* ── 读写 ── */
size_t i2c_it_read(Device *self, void *buf, size_t count) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (conf->id >= I2C_MAX || count == 0) return 0;
    i2c_xfer_state_t *s = i2c->i2c_xfer;
    if (!s || s->active) return 0;
    s->rx_buf       = (uint8_t *)buf;
    s->rx_total_len = (uint16_t)count;
    s->index        = 0;
    s->active       = true;
    s->direction    = I2C_RX;
    hal_i2c_receive_it_start(conf->id);
    self->fun->trigger_event(self, I2C_RX_START, self->arg);
    return count;
}

void i2c_it_write(Device *self, const void *buf, size_t count) {
    I2c *i2c = GET_I2C(self);
    const i2c_config_t *conf = self->info->conf;
    if (conf->id >= I2C_MAX || count == 0) return;
    i2c_xfer_state_t *s = i2c->i2c_xfer;
    if (!s || s->active) return;
    s->tx_buf       = (const uint8_t *)buf;
    s->tx_total_len = (uint16_t)count;
    s->index        = 0;
    s->active       = true;
    s->direction    = I2C_TX;
    hal_i2c_transmit_it_start(conf->id);
    self->fun->trigger_event(self, I2C_TX_START, self->arg);
}

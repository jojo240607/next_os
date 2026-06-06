/**
 * I2C 驱动 — Device VTable + SVC 模式
 *
 * 用户接口（通过 SVC 调用）:
 *   dev_ioctl(i2c, I2C_IOCTL_SET_ADDR, &addr)  → 设置从设备地址
 *   dev_write(i2c, buf, count)  → I2C 主设备发送 (IT/DMA/阻塞)
 *   dev_read(i2c, buf, count)   → I2C 主设备接收 (IT/DMA/阻塞)
 *   dev_ioctl(i2c, I2C_IOCTL_TRANSFER, &args) → 写寄存器地址+读数据组合
 *
 * 内部模式选择:
 *   dma_cfg != NULL → DMA 模式
 *   dma_cfg == NULL && it_enable → 中断模式
 *   否则 → 阻塞轮询 (调用 hal 层实现)
 */
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_i2c.h"
#include "common/rcc.h"

static bool i2c_irq_handler_impl(nvic_irq_t *irq_conf);
static bool i2c_irq_err_handler_impl(nvic_irq_t *irq_conf);

/* ── Device VTable override ── */
dev_init_override(i2c_dev_init_impl);
dev_read_override(i2c_dev_read_impl);
dev_write_override(i2c_dev_write_impl);
dev_ioctl_override(i2c_dev_ioctl_impl);

static void i2c_destroy(I2c* self);

static const I2cFun i2c_fun = {
    .destroy = i2c_destroy,
};

/* ══════════════════════════════════════════════════════════════
   构造 / 析构
   ══════════════════════════════════════════════════════════════ */

I2c* i2c_create(const device_info_t *info)
{
    I2c* obj = (I2c*)os_malloc(sizeof(I2c));
    if (obj) {
        memset(obj, 0, sizeof(I2c));
        i2c_init(obj, info);
    }
    return obj;
}

void i2c_init(I2c* self, const device_info_t *info)
{
    device_init(&self->base, info);
    self->fun = &i2c_fun;

    GET_DEVICE_VTABLE(self)->dev_init  = i2c_dev_init_impl;
    GET_DEVICE_VTABLE(self)->dev_read  = i2c_dev_read_impl;
    GET_DEVICE_VTABLE(self)->dev_write = i2c_dev_write_impl;
    GET_DEVICE_VTABLE(self)->dev_ioctl = i2c_dev_ioctl_impl;

    self->i2c_xfer  = NULL;
    self->slave_addr = 0x50;  /* 默认从地址 */
}

void i2c_deinit(I2c* self)
{
    device_deinit(GET_DEVICE(self));
}

static void i2c_destroy(I2c* self)
{
    if (self) { i2c_deinit(self); os_free(self); }
}

/* ══════════════════════════════════════════════════════════════
   dev_init — 硬件初始化
   ══════════════════════════════════════════════════════════════ */

dev_init_override(i2c_dev_init_impl)
{
    I2c *i2c = (I2c *)self;
    const i2c_config_t *conf = self->info->conf;

    if (conf->id >= I2C_MAX) return;

    /* 1. PinMux AF4 (开漏) */
    const i2c_pins_t *p = &conf->pins;
    pin_config_t pins[] = {
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_OD, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->scl_pin },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_OD, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->sda_pin },
    };
    for (int i = 0; i < 2; i++) {
        if (pinmux_request(&pins[i]) != PINMUX_SUCCESS) {
            LOG_ERROR("i2c", "pinmux error");
            return;
        }
    }

    /* 2. 时钟 + 复位 + 配置 */
    hal_i2c_clock_enable(conf->id);
    hal_i2c_reset(conf->id);
    hal_i2c_set_clock(conf->id, conf->clock_speed);
    hal_i2c_set_addr(conf->id, conf->addr_mode, conf->own_address);
    hal_i2c_enable(conf->id);

    /* 4. DMA 初始化 */
    if (conf->dma_cfg) {
        if (!i2c->i2c_xfer) {
            i2c->i2c_xfer = os_malloc(sizeof(i2c_xfer_state_t));
            memset(i2c->i2c_xfer, 0, sizeof(i2c_xfer_state_t));
            i2c->i2c_xfer->i2c_sem = semaphore_create(0);
        }
        if (conf->dma_cfg->tx_dma) dma_stream_request(conf->dma_cfg->tx_dma);
        if (conf->dma_cfg->rx_dma) dma_stream_request(conf->dma_cfg->rx_dma);
        hal_i2c_dma_init(conf->id, conf->dma_cfg->tx_dma,
                         conf->dma_cfg->rx_dma);
    } else if (hal_i2c_it_init(conf->id, conf->it_enable)) {
        self->irq_conf->arg = self;

        if (conf->it_enable & (xI2C_IT_TXE | xI2C_IT_RXNE)) {
            self->irq_conf->handler = i2c_irq_handler_impl;
            self->irq_conf->irq_list->fun->add_int(
                    self->irq_conf->irq_list, I2C1_EV_IRQ + conf->id * 2);
            self->fun->config_irq(self, self->irq_conf);
        }
        if (conf->it_enable & xI2C_IT_ERR) {
            self->irq_conf->handler = i2c_irq_err_handler_impl;
            self->irq_conf->irq_list->fun->add_int(
                    self->irq_conf->irq_list, I2C1_ER_IRQ + conf->id * 2);
            self->fun->config_irq(self, self->irq_conf);
        }

        if (!i2c->i2c_xfer) {
            i2c->i2c_xfer = os_malloc(sizeof(i2c_xfer_state_t));
            memset(i2c->i2c_xfer, 0, sizeof(i2c_xfer_state_t));
            i2c->i2c_xfer->i2c_sem = semaphore_create(0);
        }
    }

    LOG_DEBUG("i2c", "i2c%d init ok", conf->id);
}

/* ══════════════════════════════════════════════════════════════
   dev_write — I2C 主设备发送
   ══════════════════════════════════════════════════════════════ */

dev_write_override(i2c_dev_write_impl)
{
    I2c *i2c = (I2c *)self;
    const i2c_config_t *conf = self->info->conf;

    if (conf->id >= I2C_MAX || count == 0) return;

    i2c_xfer_state_t *s = i2c->i2c_xfer;
    const uint8_t *data = (const uint8_t *)buf;
    uint16_t len = (uint16_t)count;

    if (conf->dma_cfg && conf->dma_cfg->tx_dma && conf->dma_cfg->rx_dma) {
        /* ─── DMA 发送 ─── */
        i2c_transmit_dma(conf->id, conf->dma_cfg->tx_dma,
                         i2c->slave_addr, data, len);

    } else if (conf->it_enable) {
        /* ─── 中断发送 ─── */
        if (!s || s->active) {
            return;
        }

        s->tx_buf    = data;
        s->total_len = len;
        s->index     = 0;
        s->active    = true;
        s->direction = 0;  /* TX */

        hal_i2c_transmit_it_start(conf->id);
        s->i2c_sem->fun->take(s->i2c_sem);

    } else {
        /* ─── 阻塞发送 ─── */
        i2c_transmit(conf->id, i2c->slave_addr, data, len);
    }
}

/* ══════════════════════════════════════════════════════════════
   dev_read — I2C 主设备接收
   ══════════════════════════════════════════════════════════════ */

dev_read_override(i2c_dev_read_impl)
{
    I2c *i2c = (I2c *)self;
    const i2c_config_t *conf = self->info->conf;

    if (conf->id >= I2C_MAX || count == 0) return 0;

    i2c_xfer_state_t *s = i2c->i2c_xfer;
    uint8_t *data = (uint8_t *)buf;
    uint16_t len = (uint16_t)count;

    if (conf->dma_cfg && conf->dma_cfg->rx_dma) {
        /* ─── DMA 接收 ── */
        i2c_transmit_dma(conf->id, conf->dma_cfg->rx_dma,
                         i2c->slave_addr, NULL, len);  /* TODO: 正确 DMA 接收 */

    } else if (conf->it_enable) {
        /* ─── 中断接收 ─── */
        if (!s || s->active) {
            return 0;
        }

        s->rx_buf    = data;
        s->total_len = len;
        s->index     = 0;
        s->active    = true;
        s->direction = 1;  /* RX */

        hal_i2c_receive_it_start(conf->id);
        s->i2c_sem->fun->take(s->i2c_sem);

    } else {
        /* ─── 阻塞接收 ─── */
        i2c_receive(conf->id, i2c->slave_addr, data, len);
    }
    return len;
}

/* ══════════════════════════════════════════════════════════════
   dev_ioctl — 扩展控制
   ══════════════════════════════════════════════════════════════ */

dev_ioctl_override(i2c_dev_ioctl_impl)
{
    I2c *i2c = (I2c *)self;

    switch (cmd) {
    case I2C_IOCTL_SET_ADDR:
        if (arg) i2c->slave_addr = *(uint8_t *)arg;
        break;

    case I2C_IOCTL_TRANSFER: {
        /* 写寄存器地址 + 读数据的组合操作 */
        i2c_transfer_args_t *a = (i2c_transfer_args_t *)arg;
        if (!a) break;
        i2c->slave_addr = a->slave_addr;
        if (a->tx_buf && a->tx_len > 0) {
            virtual_dev_write(self, a->tx_buf, a->tx_len);
        }
        if (a->rx_buf && a->rx_len > 0) {
            virtual_dev_read(self, a->rx_buf, a->rx_len);
        }
        break;
    }
    default: break;
    }
}

/* ══════════════════════════════════════════════════════════════
   I2C 中断处理
   ══════════════════════════════════════════════════════════════ */

static bool i2c_irq_handler_impl(nvic_irq_t *irq_conf)
{
    I2c *i2c = GET_I2C(irq_conf->arg);
    const i2c_config_t *conf = GET_DEVICE(i2c)->info->conf;
    i2c_xfer_state_t *s = i2c->i2c_xfer;
    if (!s || !s->active) return true;

    uint16_t sr1 = hal_i2c_get_it_event(conf->id);

    /* SB: 起始条件已发送 → 发送从设备地址 */
    if (sr1 & I2C_FLG_SB) {
        *hal_i2c_addr(conf->id) = (i2c->slave_addr << 1)
            | (s->direction ? 0x01 : 0x00);
        return true;
    }

    /* ADDR: 地址已发送 → 清 ADDR */
    if (sr1 & I2C_FLG_ADDR) {
        hal_i2c_clear_addr_flag(conf->id);
        if (s->direction == 1 && s->total_len == 1) {
            hal_i2c_close_ack(conf->id);
        }
        return true;
    }

    /* TXE: 发送下一个字节 */
    if (sr1 & I2C_FLG_TXE) {
        if (s->direction == 0 && s->index < s->total_len) {
            *hal_i2c_addr(conf->id) = s->tx_buf[s->index++];
        } else if (s->direction == 0) {
            hal_i2c_clear_it_event(conf->id, xI2C_IE_ITBUFEN);
        }
    }

    /* RXNE: 收到一个字节 */
    if (sr1 & I2C_FLG_RXNE) {
        if (s->direction == 1 && s->index < s->total_len) {
            s->rx_buf[s->index++] = *hal_i2c_addr(conf->id);
            if (s->index == s->total_len - 1) {
                hal_i2c_close_ack(conf->id);
            }
            if (s->index >= s->total_len) {
                hal_i2c_clear_it_event(conf->id,
                    xI2C_IE_ITBUFEN | xI2C_IE_ITEVTEN);
                hal_i2c_stop(conf->id);
                s->active = false;
                s->i2c_sem->fun->give(s->i2c_sem);
            }
        }
        return true;
    }

    /* BTF: 字节传输完成 → 发 STOP */
    if (sr1 & I2C_FLG_BTF) {
        if (s->direction == 0 && s->index >= s->total_len) {
            hal_i2c_stop(conf->id);
            hal_i2c_clear_it_event(conf->id,
                xI2C_IE_ITEVTEN | xI2C_IE_ITBUFEN);
            s->active = false;
            s->i2c_sem->fun->give(s->i2c_sem);
        }
        return true;
    }

    /* 防御：SR1 == 0 可能是噪声 */
    if (sr1 == 0) {
        hal_i2c_stop(conf->id);
        hal_i2c_clear_it_event(conf->id,
            xI2C_IE_ITEVTEN | xI2C_IE_ITBUFEN);
    }
    return true;
}

static bool i2c_irq_err_handler_impl(nvic_irq_t *irq_conf)
{
    I2c *i2c = GET_I2C(irq_conf->arg);
    LOG_ERROR("i2c", "irq error");
    if (i2c->i2c_xfer && i2c->i2c_xfer->active) {
        i2c->i2c_xfer->active = false;
        i2c->i2c_xfer->i2c_sem->fun->give(i2c->i2c_xfer->i2c_sem);
    }
    return true;
}

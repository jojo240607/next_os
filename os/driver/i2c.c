#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_i2c.h"
#include "common/rcc.h"

//static void i2c_transmit_dma(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
static void i2c_receive_it(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
static void i2c_transmit_it(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
//static void i2c_receive(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
//static void i2c_transmit(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);

dev_init_override(i2c_dev_init_impl);
static bool i2c_irq_handler_impl(void *arg);
static bool i2c_irq_err_handler_impl(void *arg);
// 析构函数声明
static void i2c_destroy(I2c* self);


// TODO: 初始化数据成员
static const I2cFun i2c_fun = {
    .destroy = i2c_destroy,
//	.transmit = i2c_transmit,
//	.receive = i2c_receive,
	.transmit_it = i2c_transmit_it,
	.receive_it = i2c_receive_it,
};
// 构造函数实现
I2c* i2c_create(const i2c_config_t *conf, const dev_pripority_t *priority) {
    I2c* obj = (I2c*)os_malloc(sizeof(I2c));
    if (obj) {
        memset(obj, 0, sizeof(I2c));
        i2c_init(obj, conf, priority);
    }
    return obj;
}

void i2c_init(I2c* self, const i2c_config_t *conf, const dev_pripority_t *priority) {
    // 初始化基类部分
    device_init(&self->base, priority);
    self->fun = &(i2c_fun);
    // TODO: 初始化派生类特有成员
    self->conf = conf;
    
	def_dev_init(self) = i2c_dev_init_impl;
    self->i2c_tx_sem = semaphore_create(0);
    self->i2c_rx_sem = semaphore_create(0);
}



void i2c_deinit(I2c* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void i2c_destroy(I2c* self) {
    if (self != NULL) {
        i2c_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(i2c_dev_init_impl) {
    LOG_DEBUG("i2c", "i2c init");
    // TODO: add dev_init method
    I2c *i2c = (I2c *)self;
    //params 
    if (i2c->conf->id >= I2C_MAX) {
        return;
    }

    /* 1. 通过 PinMux 申请引脚 (AF4) */
    const i2c_pins_t *p = &i2c->conf->pins;
    /*
     *  I2C1：例如 PB6、PB7 等引脚，复用功能为 AF4。
        I2C2：例如 PB10、PB11 等引脚，复用功能为 AF4。
        I2C3：例如 PA8、PC9 等引脚，复用功能也为 AF4。
        */
    pin_config_t pins[] = {
            {.mode = PIN_MODE_AF,
              .otype = PIN_OTYPE_OD,
              .ospeed = PIN_OSPEED_HIGH,
              .pupd = PIN_PUPD_NONE,
              .af = p->scl_pin },
            {.mode = PIN_MODE_AF,
              .otype = PIN_OTYPE_OD,
              .ospeed = PIN_OSPEED_HIGH,
              .pupd = PIN_PUPD_NONE,
              .af = p->sda_pin }
    };
    for (int i = 0; i < 2; i++) {
        if (pinmux_request(&pins[i]) != PINMUX_SUCCESS) {
            LOG_ERROR("i2c", "i2c pinmux error");
            return;
        }
    }
    LOG_DEBUG("i2c", "enable clock");
    /* 2. 使能时钟 */
    hal_i2c_clock_enable(i2c->conf->id);
    /* 3. 复位 I2C */
    hal_i2c_reset(i2c->conf->id);

    /* 4. 配置时钟 */

    hal_i2c_set_clock(i2c->conf->id, i2c->conf->clock_speed);

    /* 5. 配置本机地址 */
    hal_i2c_set_addr(i2c->conf->id, i2c->conf->addr_mode, i2c->conf->own_address);

    /* 6. 使能外设 */
    hal_i2c_enable(i2c->conf->id);

    /* 7. 中断配置 */
    if (hal_i2c_it_init(i2c->conf->id, i2c->conf->it_enable)) {
        self->irq_conf.semaphore = sem;
        self->irq_conf.arg = self;
        if (i2c->conf->it_enable & (xI2C_IT_TXE | xI2C_IT_RXNE)) {
            self->irq_conf.handler = i2c_irq_handler_impl;
            self->irq_conf.irq_num = I2C1_EV_IRQ + i2c->conf->id * 2;
            self->fun->attach_irq(self, &self->irq_conf);
        }
        if (i2c->conf->it_enable & xI2C_IT_ERR) {
            self->irq_conf.handler = i2c_irq_err_handler_impl;
            self->irq_conf.irq_num = I2C1_ER_IRQ + i2c->conf->id * 2;
            self->fun->attach_irq(self, &self->irq_conf);
        }
    }

    if (i2c->conf->dma_cfg) {
        if (i2c->conf->dma_cfg->tx_dma) {
            dma_stream_request(i2c->conf->dma_cfg->tx_dma);
        }
        if (i2c->conf->dma_cfg->rx_dma) {
            dma_stream_request(i2c->conf->dma_cfg->rx_dma);
        }
        hal_i2c_dma_init(i2c->conf->id, i2c->conf->dma_cfg->tx_dma, i2c->conf->dma_cfg->rx_dma);
    }
}


static bool i2c_irq_handler_impl(void *arg) {
    I2c *i2c = GET_I2C(arg);

   // xI2C_TypeDef *i2c_ctrl = I2Cx[i2c->conf->id];
    i2c_xfer_state_t *s = &i2c->i2c_xfer;
    if (!s->active) {
        return true;
    }

    uint16_t sr1 = hal_i2c_get_it_event(i2c->conf->id);

    /* SB: 起始条件已发送 */
    if (sr1 & I2C_FLG_SB) {
        if (s->direction == 0) {  // TX
            *hal_i2c_addr(i2c->conf->id) = (s->slave_addr << 1) | 0x00;  // 写地址;
        } else {                    // RX
            *hal_i2c_addr(i2c->conf->id) = (s->slave_addr << 1) | 0x01;  // 读地址
        }
        return true;
    }

    /* ADDR: 地址已发送，读 SR2 清除 ADDR 标志 */
    if (sr1 & I2C_FLG_ADDR) {
        hal_i2c_clear_addr_flag(i2c->conf->id);
        if (s->direction == 1 && s->total_len == 1) {
            // 若只收一个字节，提前关闭 ACK
            hal_i2c_close_ack(i2c->conf->id);
        }
        return true;
    }

    /* TXE: 数据寄存器空，可以继续发送 */
    if (sr1 & I2C_FLG_TXE) {
        if (s->direction == 0 && s->index < s->total_len) {
            *hal_i2c_addr(i2c->conf->id) = s->tx_buf[s->index++];
        } else if (s->direction == 0 && s->index >= s->total_len) {
            // 发送完成，等 BTF 后发停止
            hal_i2c_clear_it_event(i2c->conf->id, xI2C_IE_ITBUFEN); // 关闭 TXE 中断
        }
        //return true;
    }

    /* RXNE: 收到数据 */
    if (sr1 & I2C_FLG_RXNE) {
        if (s->direction == 1 && s->index < s->total_len) {
            s->rx_buf[s->index++] = *hal_i2c_addr(i2c->conf->id);
            if (s->index == s->total_len - 1) {
                hal_i2c_close_ack(i2c->conf->id);// 最后字节前关 ACK
            }
            if (s->index >= s->total_len) {
                // 接收完成
                hal_i2c_clear_it_event(i2c->conf->id, xI2C_IE_ITBUFEN | xI2C_IE_ITEVTEN);
                hal_i2c_stop(i2c->conf->id);
                s->active = false;
                i2c->i2c_rx_sem->fun->give(i2c->i2c_rx_sem);
            }
        }
        return true;
    }

    /* BTF: 字节传输完成 */
    if (sr1 & I2C_FLG_BTF) {
        if (s->direction == 0 && s->index >= s->total_len) {
            hal_i2c_stop(i2c->conf->id); // 发 STOP
            hal_i2c_clear_it_event(i2c->conf->id, xI2C_IE_ITEVTEN | xI2C_IE_ITBUFEN);          // ★ 关掉事件中断总开关

            s->active = false;
            i2c->i2c_tx_sem->fun->give(i2c->i2c_tx_sem);
        }
        return true;
    }

    if (sr1 == 0) {
        hal_i2c_stop(i2c->conf->id);                // 发 STOP
        hal_i2c_clear_it_event(i2c->conf->id, xI2C_IE_ITEVTEN | xI2C_IE_ITBUFEN);          // ★ 关掉事件中断总开关
    }
}

static bool i2c_irq_err_handler_impl(void *arg) {
    LOG_ERROR("i2c", "irq error");
}


// transmit_it method
static void i2c_transmit_it(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len) {
    if (self->conf->id >= I2C_MAX || len == 0) {
        return;
    }

    i2c_xfer_state_t *s = &self->i2c_xfer;
    s->tx_buf = data;
    s->total_len = len;
    s->index = 0;
    s->active = true;
    s->direction = 0;   // TX
    s->slave_addr = slave_addr;

    hal_i2c_transmit_it_start(self->conf->id);
    if (self->i2c_tx_sem) {
        self->i2c_tx_sem->fun->take(self->i2c_tx_sem);
    }
}


// receive_it method
static void i2c_receive_it(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len) {
    // TODO: add receive_it method
    if (self->conf->id >= I2C_MAX || len == 0) {
        return;
    }

    i2c_xfer_state_t *s = &self->i2c_xfer;
    s->rx_buf = buffer;
    s->total_len = len;
    s->index = 0;
    s->active = true;
    s->direction = 1;   // RX
    s->slave_addr = slave_addr;

    hal_i2c_receive_it_start(self->conf->id);
    if (self->i2c_rx_sem) {
        self->i2c_rx_sem->fun->take(self->i2c_rx_sem);
    }
}


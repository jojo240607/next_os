#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_i2c.h"

static void i2c_transmit_dma(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
static void i2c_receive_it(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
static void i2c_transmit_it(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
static void i2c_receive(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
static void i2c_transmit(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);

dev_init_override(i2c_dev_init_impl);
dev_read_override(i2c_dev_read_impl);
dev_write_override(i2c_dev_write_impl);
dev_ioctl_override(i2c_dev_ioctl_impl);
static bool i2c_irq_handler_impl(void *arg);
static bool i2c_irq_err_handler_impl(void *arg);
// 析构函数声明
static void i2c_destroy(I2c* self);


// TODO: 初始化数据成员
static const I2cFun i2c_fun = {
    .destroy = i2c_destroy,
	.transmit = i2c_transmit,
	.receive = i2c_receive,
	.transmit_it = i2c_transmit_it,
	.receive_it = i2c_receive_it,
	.transmit_dma = i2c_transmit_dma,
};
// 构造函数实现
I2c* i2c_create(const i2c_config_t *conf) {
    I2c* obj = (I2c*)os_malloc(sizeof(I2c));
    if (obj) {
        memset(obj, 0, sizeof(I2c));
        i2c_init(obj, conf);
    }
    return obj;
}

void i2c_init(I2c* self, const i2c_config_t *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(i2c_fun);
    // TODO: 初始化派生类特有成员
    self->conf = conf;
    
	def_dev_init(self) = i2c_dev_init_impl;
	def_dev_read(self) = i2c_dev_read_impl;
	def_dev_write(self) = i2c_dev_write_impl;
	def_dev_ioctl(self) = i2c_dev_ioctl_impl;
    self->i2c_tx_sem = semaphore_create(0);
    self->i2c_rx_sem = semaphore_create(0);
}

static void hal_i2c_set_clock(xI2C_TypeDef *i2c, uint32_t pclk, uint32_t target_speed)
{
    /* 配置 CR2 的 FREQ 字段 (单位 MHz) */
    uint32_t freq = pclk / 1000000;
    i2c->CR2 = (i2c->CR2 & ~0x3F) | (freq & 0x3F);

    /* 计算 CCR */
    uint32_t ccr;
    if (target_speed <= 100000) {
        // 标准模式
        ccr = pclk / (target_speed * 2);
        if (ccr < 4) ccr = 4;
        i2c->CCR = ccr & 0xFFF;
    } else {
        // 快速模式 (占空比 16:9)
        ccr = pclk / (target_speed * 25);
        if (ccr < 1) ccr = 1;
        i2c->CCR = (1 << 15) | (ccr & 0xFFF);
    }

    /* 配置 TRISE */
    if (target_speed <= 100000) {
        i2c->TRISE = freq + 1;
    } else {
        i2c->TRISE = (freq * 300) / 1000 + 1;
    }
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

    xI2C_TypeDef *i2c_ctrl = I2Cx[i2c->conf->id];

    /* 3. 复位 I2C */
    i2c_ctrl->CR1 = (1 << 15);  /* SWRST */
    i2c_ctrl->CR1 = 0;

    /* 4. 配置时钟 */
    uint32_t pclk = 42000000;  /* APB1 时钟 42MHz */
    hal_i2c_set_clock(i2c_ctrl, pclk, i2c->conf->clock_speed);

    /* 5. 配置本机地址 */
    if (i2c->conf->addr_mode == I2C_ADDR_7BIT) {
        i2c_ctrl->OAR1 = (i2c->conf->own_address & 0x7F) << 1;
        i2c_ctrl->OAR1 &= ~(1 << 15);  /* 7 位模式 */
    } else {
        i2c_ctrl->OAR1 = (i2c->conf->own_address & 0x3FF) | (1 << 15);
    }

    /* 6. 使能外设 */
    i2c_ctrl->CR1 |= xI2C_CR1_ACK | xI2C_CR1_PE;

    /* 7. 中断配置 */
    if (i2c->conf->it_enable) {
        uint32_t cr2 = 0;
        self->irq_conf.semaphore = sem;
        self->irq_conf.arg = self;
        if (i2c->conf->it_enable & (xI2C_IT_TXE | xI2C_IT_RXNE)) {
            cr2 |= xI2C_CR2_ITBUFEN | xI2C_CR2_ITEVTEN;
            self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
            self->irq_conf.handler = i2c_irq_handler_impl;
            self->irq_conf.irq_num = I2C1_EV_IRQ + i2c->conf->id * 2;
            self->fun->attach_irq(self, &self->irq_conf);
        }
        if (i2c->conf->it_enable & xI2C_IT_ERR) {
            cr2 |= xI2C_CR2_ITERREN;
            self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
            self->irq_conf.handler = i2c_irq_err_handler_impl;
            self->irq_conf.irq_num = I2C1_ER_IRQ + i2c->conf->id * 2;
            self->fun->attach_irq(self, &self->irq_conf);
        }
        i2c_ctrl->CR2 = cr2;
    }

    if (i2c->conf->dma_cfg && i2c->conf->dma_cfg->tx_dma) {
        dma_stream_request(i2c->conf->dma_cfg->tx_dma);
        // 保存配置到内部数组
        i2c_ctrl->CR2 |= (1 << 11);   // 使能发送 DMA (TXDMAEN)
    }
    if (i2c->conf->dma_cfg && i2c->conf->dma_cfg->rx_dma) {
        dma_stream_request(i2c->conf->dma_cfg->rx_dma);
        i2c_ctrl->CR2 |= (1 << 12);   // 使能接收 DMA (RXDMAEN)
    }

}
// dev_read method
dev_read_override(i2c_dev_read_impl) {
    // TODO: add dev_read method
    //I2c *i2c = (I2c *)self;
    //params , void *buf, size_t count
    
}
// dev_write method
dev_write_override(i2c_dev_write_impl) {
    // TODO: add dev_write method
    //I2c *i2c = (I2c *)self;
    //params , const void *buf, size_t count
    
}
// dev_ioctl method
dev_ioctl_override(i2c_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    //I2c *i2c = (I2c *)self;
    //params , int cmd, void *arg
    
}


static bool i2c_irq_handler_impl(void *arg) {
    I2c *i2c = GET_I2C(arg);

    xI2C_TypeDef *i2c_ctrl = I2Cx[i2c->conf->id];
    i2c_xfer_state_t *s = &i2c->i2c_xfer;
    if (!s->active) {
        return true;
    }

    uint16_t sr1 = i2c_ctrl->SR1;

    /* SB: 起始条件已发送 */
    if (sr1 & xI2C_SR1_SB) {
        if (s->direction == 0)  // TX
            i2c_ctrl->DR = (s->slave_addr << 1) | 0x00;  // 写地址
        else                    // RX
            i2c_ctrl->DR = (s->slave_addr << 1) | 0x01;  // 读地址
        return true;
    }

    /* ADDR: 地址已发送，读 SR2 清除 ADDR 标志 */
    if (sr1 & xI2C_SR1_ADDR) {
        uint16_t sr2 = i2c_ctrl->SR2;
        if (s->direction == 1 && s->total_len == 1) {
            // 若只收一个字节，提前关闭 ACK
            i2c_ctrl->CR1 &= ~xI2C_CR1_ACK;
        }
        return true;
    }

    /* TXE: 数据寄存器空，可以继续发送 */
    if (sr1 & xI2C_SR1_TXE) {
        if (s->direction == 0 && s->index < s->total_len) {
            i2c_ctrl->DR = s->tx_buf[s->index++];
        } else if (s->direction == 0 && s->index >= s->total_len) {
            // 发送完成，等 BTF 后发停止
            i2c_ctrl->CR2 &= ~(xI2C_CR2_ITBUFEN);  // 关闭 TXE 中断
        }
        //return true;
    }

    /* RXNE: 收到数据 */
    if (sr1 & xI2C_SR1_RXNE) {
        if (s->direction == 1 && s->index < s->total_len) {
            s->rx_buf[s->index++] = i2c_ctrl->DR;
            if (s->index == s->total_len - 1) {
                i2c_ctrl->CR1 &= ~xI2C_CR1_ACK;  // 最后字节前关 ACK
            }
            if (s->index >= s->total_len) {
                // 接收完成
                i2c_ctrl->CR2 &= ~(xI2C_CR2_ITBUFEN | xI2C_CR2_ITEVTEN);
                i2c_ctrl->CR1 |= xI2C_CR1_STOP;
                s->active = false;
                i2c->i2c_rx_sem->fun->give(i2c->i2c_rx_sem);
            }
        }
        return true;
    }

    /* BTF: 字节传输完成 */
    if (sr1 & xI2C_SR1_BTF) {
        if (s->direction == 0 && s->index >= s->total_len) {
            i2c_ctrl->CR1 |= xI2C_CR1_STOP;                // 发 STOP
            i2c_ctrl->CR2 &= ~(xI2C_CR2_ITEVTEN | xI2C_CR2_ITBUFEN);          // ★ 关掉事件中断总开关
            s->active = false;
            i2c->i2c_tx_sem->fun->give(i2c->i2c_tx_sem);
        }
        return true;
    }

    if (sr1 == 0) {
        i2c_ctrl->CR1 |= xI2C_CR1_STOP;                // 发 STOP
        i2c_ctrl->CR2 &= ~(xI2C_CR2_ITEVTEN | xI2C_CR2_ITBUFEN);          // ★ 关掉事件中断总开关
    }
}

static bool i2c_irq_err_handler_impl(void *arg) {
    LOG_ERROR("i2c", "irq error");
}


// transmit method
static void i2c_transmit(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len) {
    // TODO: add transmit method
    if (self->conf->id >= I2C_MAX) {
        return;
    }
    xI2C_TypeDef *i2c_ctrl = I2Cx[self->conf->id];

    /* 发送起始条件 */
    i2c_ctrl->CR1 |= xI2C_CR1_START;
    while (!(i2c_ctrl->SR1 & xI2C_SR1_SB));
    /* 发送地址 (写) */
    i2c_ctrl->DR = (slave_addr << 1) | 0x00;   /* LSB=0 表示写 */
    while (!(i2c_ctrl->SR1 & xI2C_SR1_ADDR));
    (void)i2c_ctrl->SR2;   /* 读 SR2 清除 ADDR 标志 */

    /* 发送数据 */
    for (int i = 0; i < len; i++) {
        while (!(i2c_ctrl->SR1 & xI2C_SR1_TXE));
        i2c_ctrl->DR = data[i];
    }
    /* 等待传输完成 */
    while (!(i2c_ctrl->SR1 & xI2C_SR1_BTF));
    /* 发送停止条件 */
    i2c_ctrl->CR1 |= xI2C_CR1_STOP;
}


// receive method
static void i2c_receive(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len) {
    // TODO: add receive method
    if (self->conf->id >= I2C_MAX) {
        return;
    }
    xI2C_TypeDef *i2c_ctrl = I2Cx[self->conf->id];

    /* 发送起始条件 */
    i2c_ctrl->CR1 |= xI2C_CR1_START;
    while (!(i2c_ctrl->SR1 & xI2C_SR1_SB));
    /* 发送地址 (读) */
    i2c_ctrl->DR = (slave_addr << 1) | 0x01;   /* LSB=1 表示读 */
    while (!(i2c_ctrl->SR1 & xI2C_SR1_ADDR));
    (void)i2c_ctrl->SR2;

    /* 接收数据 */
    for (int i = 0; i < len; i++) {
        if (i == len - 1) {
            /* 最后一个字节前关闭 ACK */
            i2c_ctrl->CR1 &= ~xI2C_CR1_ACK;
        }
        while (!(i2c_ctrl->SR1 & xI2C_SR1_RXNE));
        buffer[i] = i2c_ctrl->DR;
    }
    /* 发送停止条件 */
    i2c_ctrl->CR1 |= xI2C_CR1_STOP;

    /* 恢复 ACK */
    i2c_ctrl->CR1 |= xI2C_CR1_ACK;
}


// transmit_it method
static void i2c_transmit_it(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len) {
    if (self->conf->id >= I2C_MAX || len == 0) {
        return;
    }

    xI2C_TypeDef *i2c_ctrl = I2Cx[self->conf->id];
    i2c_xfer_state_t *s = &self->i2c_xfer;
    s->tx_buf = data;
    s->total_len = len;
    s->index = 0;
    s->active = true;
    s->direction = 0;   // TX
    s->slave_addr = slave_addr;

    /* 使能中断（若未打开） */
    i2c_ctrl->CR2 |= xI2C_CR2_ITBUFEN | xI2C_CR2_ITEVTEN;

    /* 发送起始条件 */
    i2c_ctrl->CR1 |= xI2C_CR1_START;
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

    xI2C_TypeDef *i2c_ctrl = I2Cx[self->conf->id];
    i2c_xfer_state_t *s = &self->i2c_xfer;
    s->rx_buf = buffer;
    s->total_len = len;
    s->index = 0;
    s->active = true;
    s->direction = 1;   // RX
    s->slave_addr = slave_addr;

    i2c_ctrl->CR2 |= xI2C_CR2_ITBUFEN | xI2C_CR2_ITEVTEN;
    i2c_ctrl->CR1 |= xI2C_CR1_START;
    if (self->i2c_rx_sem) {
        self->i2c_rx_sem->fun->take(self->i2c_rx_sem);
    }
}


// transmit_dma method
static void i2c_transmit_dma(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len) {
    // TODO: add transmit_dma method
    if (self->conf->id >= I2C_MAX) {
        return;
    }
    xI2C_TypeDef *i2c_ctrl = I2Cx[self->conf->id];
    // 初始化 DMA 传输
    dma_start_transfer(self->conf->dma_cfg->tx_dma,
                       (uint32_t)data, (uint32_t)&i2c_ctrl->DR, len);
    // 发送起始条件，地址等由软件完成第一个字节，之后 DMA 接管
    i2c_ctrl->CR1 |= xI2C_CR1_START;
    while (!(i2c_ctrl->SR1 & xI2C_SR1_SB));
    i2c_ctrl->DR = (slave_addr << 1) | 0x00;  // 写地址
    while (!(i2c_ctrl->SR1 & xI2C_SR1_ADDR));  // 等待地址发送完毕
    (void)i2c_ctrl->SR2;
}


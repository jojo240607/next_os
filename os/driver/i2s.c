#include "i2s.h"
#include <stdio.h>
#include <stdlib.h>
#include "common/rcc.h"
#include "../common/linear_pool.h"

dev_init_override(i2s_dev_init_impl);

// 析构函数声明
static void i2s_destroy(I2s* self);
static bool i2s_irq_handler_impl(void *arg);

// TODO: 初始化数据成员
static const I2sFun i2s_fun = {
    .destroy = i2s_destroy,
};
// 构造函数实现
I2s* i2s_create(const i2s_config_t *conf) {
    I2s* obj = (I2s*)os_malloc(sizeof(I2s));
    if (obj) {
        memset(obj, 0, sizeof(I2s));
        i2s_init(obj, conf);
    }
    return obj;
}

void i2s_init(I2s* self, const i2s_config_t *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(i2s_fun);
    // TODO: 初始化派生类特有成员
    self->conf = conf;
	def_dev_init(self) = i2s_dev_init_impl;
    self->i2s_xfer.i2s_rx_sem = semaphore_create(0);
    self->i2s_xfer.i2s_tx_sem = semaphore_create(0);
}

void i2s_deinit(I2s* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void i2s_destroy(I2s* self) {
    if (self != NULL) {
        i2s_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(i2s_dev_init_impl) {
    // TODO: add dev_init method
    I2s *i2s = (I2s *)self;
    //params 
    if (!i2s->conf || i2s->conf->id >= I2S_MAX) {
        return;
    }

    // 1. 使能时钟 (APB1)
    if (i2s->conf->id == I2S_2) {
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2S2EN);  // /I2S2
    } else {
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2S3EN);  // SPI3/I2S3
    }
    // 2. 配置引脚 (AF5)
    const i2s_pins_t *p = &i2s->conf->pins;
    pin_config_t pins[4] = {
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->sck_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->sd_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->ws_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = p->mck_pin },
    };
    if (pinmux_request_group(pins, i2s->conf->enable_mck ? 4 : 3) != 0) {
        return;
    }

    // 3. 配置 PLLI2S (产生音频时钟)
    // PLLI2S VCO = HSE / PLLM * PLLI2SN (例: 8MHz / 8 * 258 = 258MHz)
    // I2S_CLK = VCO / PLLI2SR (例: 258MHz / 3 = 86MHz)
    volatile uint32_t *PLLI2SCFGR = (uint32_t*)0x40023884UL;
    *PLLI2SCFGR = (i2s->conf->plli2s_r & 0x7) << 28 | (i2s->conf->plli2s_n & 0x1FF) << 6;
    // 使能 PLLI2S
    volatile uint32_t *CR = (uint32_t*)0x40023800UL;
    *CR |= (1 << 26);
    while (!(*CR & (1 << 27)));

    xI2S_TypeDef *i2s_ctrl = I2Sx[i2s->conf->id];

    // 4. 配置 I2S 预分频 (SPI_I2SPR)
    uint32_t i2spr = 0;
    i2spr |= (i2s->conf->i2s_div & 0xFF) << 0;
    if (i2s->conf->odd_factor) {
        i2spr |= xI2S_I2SPR_ODD;
    }
    if (i2s->conf->enable_mck) {
        i2spr |= xI2S_I2SPR_MCKOE;
    }
    i2s_ctrl->I2SPR = i2spr;

    // 5. 配置 I2S 控制寄存器 (SPI_I2SCFGR)
    uint32_t i2scfgr = xI2S_I2SCFGR_I2SMOD;   // 切换到 I2S 模式
    i2scfgr |= (i2s->conf->mode & 0x3) << 8;       // I2SCFG
    i2scfgr |= (i2s->conf->standard & 0x3) << 4;   // I2SSTD
    if (i2s->conf->standard == xI2S_STANDARD_PCM_LONG) {
        i2scfgr |= xI2S_I2SCFGR_PCMSYNC;
    }
    i2scfgr |= (i2s->conf->clock_polarity & 0x1) << 3;  // CKPOL
    i2scfgr |= (i2s->conf->data_format & 0x3) << 1;     // DATLEN
    if (i2s->conf->data_format != I2S_DATA_16BIT) {
        i2scfgr |= xI2S_I2SCFGR_CHLEN;             // 32位通道长度
    }
    i2s_ctrl->I2SCFGR = i2scfgr;

    // 6. 中断配置
    if (i2s->conf->it_enable) {
        //i2s_callbacks[i2s->conf->id] = i2s->conf->callback;
        uint32_t cr2 = i2s_ctrl->CR2;
        if (i2s->conf->it_enable & xI2S_IT_TXE)  {
            cr2 |= xI2S_CR2_TXEIE;
        }
        if (i2s->conf->it_enable & xI2S_IT_RXNE) {
            cr2 |= xI2S_CR2_RXNEIE;
        }
        if (i2s->conf->it_enable & xI2S_IT_ERR) {
            cr2 |= xI2S_CR2_ERRIE;
        }
        i2s_ctrl->CR2 = cr2;

        self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
        self->irq_conf.handler = i2s_irq_handler_impl;
        self->irq_conf.semaphore = sem;
        self->irq_conf.arg = self;
        self->irq_conf.irq_num = SPI2_IRQ + i2s->conf->id;
        self->fun->attach_irq(self, &self->irq_conf);
    }

    // 7. DMA 配置
    if (i2s->conf->dma_cfg && i2s->conf->dma_cfg->tx_dma) {
        if (dma_stream_request(i2s->conf->dma_cfg->tx_dma) != 0) {
            return;
        }
        //i2s_tx_dma_i2s->confs[i2s->conf->id] = *i2s->conf->tx_dma_i2s->conf;
        //i2s_tx_dma_used[i2s->conf->id] = true;
        i2s_ctrl->CR2 |= xI2S_CR2_TXDMAEN;
    }
    if (i2s->conf->dma_cfg && i2s->conf->dma_cfg->rx_dma) {
        if (dma_stream_request(i2s->conf->dma_cfg->rx_dma) != 0) {
            return ;
        }
        //i2s_rx_dma_i2s->confs[i2s->conf->id] = *i2s->conf->rx_dma_i2s->conf;
        //i2s_rx_dma_used[i2s->conf->id] = true;
        i2s_ctrl->CR2 |= xI2S_CR2_RXDMAEN;
    }
}

static bool i2s_irq_handler_impl(void *arg) {
    I2s *i2s = (I2s *)arg;
    if (i2s->conf->id >= I2S_MAX) {
        return true;
    }

    xI2S_TypeDef *i2s_ctrl = I2Sx[i2s->conf->id];
    i2s_xfer_t *x = &i2s->i2s_xfer;
    uint16_t sr = i2s_ctrl->SR;

    // --- 错误处理 (优先) ---
    // I2S 可能有溢出标志 (OVR)，需参考 SPI 的做法
    if (sr & (1 << 6)) {  // OVR (过载标志)
        // 清除：读 DR 后读 SR
        volatile uint16_t dummy __attribute__((unused)) = i2s_ctrl->DR;
        (void)dummy;
        // 如果严重错误，可以关闭 I2S 再重启
        i2s_ctrl->I2SCFGR &= ~xI2S_I2SCFGR_I2SE;
        i2s_ctrl->I2SCFGR |= xI2S_I2SCFGR_I2SE;

        x->active = false;
        i2s_ctrl->CR2 &= ~(xI2S_CR2_TXEIE | xI2S_CR2_RXNEIE);
        //if (i2s_callbacks[id])
        //    i2s_callbacks[id](id, I2S_EVT_ERROR);
        return true;
    }

    // --- 接收中断 (RXNE) ---
    if ((sr & xI2S_SR_RXNE) && (i2s_ctrl->CR2 & xI2S_CR2_RXNEIE)) {
        uint16_t data = (uint16_t)i2s_ctrl->DR;   // 读 DR 自动清标志
        if (x->rx_buf && x->rx_index < x->total_len) {
            x->rx_buf[x->rx_index] = data;
        }
        x->rx_index++;
    }

    // --- 发送中断 (TXE) ---
    if ((sr & xI2S_SR_TXE) && (i2s_ctrl->CR2 & xI2S_CR2_TXEIE)) {
        if (x->tx_index < x->total_len) {
            uint16_t data = x->tx_buf ? x->tx_buf[x->tx_index] : 0x0000;
            i2s_ctrl->DR = data;
            x->tx_index++;
        } else {
            i2s_ctrl->CR2 &= ~xI2S_CR2_TXEIE;
            if (x->tx_buf != NULL && x->rx_buf == NULL) {
                //单纯发送才会用到这个信号量
                i2s->i2s_xfer.i2s_tx_sem->fun->give(i2s->i2s_xfer.i2s_tx_sem);
            }
        }
    }

    // --- 传输完成检查 ---
    // 注意：全双工时收发长度一致，这里分别检查
    bool tx_done = (!x->tx_buf) || (x->tx_index >= x->total_len);
    bool rx_done = (!x->rx_buf) || (x->rx_index >= x->total_len);

    if (tx_done && rx_done && x->active) {
        x->active = false;
        i2s_ctrl->CR2 &= ~(xI2S_CR2_TXEIE | xI2S_CR2_RXNEIE);  // 关闭发送和接收中断
        if (x->rx_buf != NULL) {
            //单纯接收，或者收发都会用到这个信号量
            i2s->i2s_xfer.i2s_rx_sem->fun->give(i2s->i2s_xfer.i2s_rx_sem);
        }
    }
}


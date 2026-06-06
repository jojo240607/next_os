#include "adc.h"
#include "../common/linear_pool.h"
#include "../log/log.h"
#include <stdio.h>
#include <stdlib.h>
#include "hal/hal_adc.h"

dev_read_override(adc_dev_read_impl);
dev_write_override(adc_dev_write_impl);
dev_ioctl_override(adc_dev_ioctl_impl);

static int adc_start_dma(Adc* self, uint16_t *buffer, uint16_t count);
static bool adc_txdma_irq_handler_impl(nvic_irq_t *irq_conf);

dev_init_override(adc_dev_init_impl);
dev_read_override(adc_dev_read_impl);

// 析构函数声明
static void adc_destroy(Adc* self);

// TODO: 初始化数据成员
static const AdcFun adc_fun = {
    .destroy = adc_destroy,
	.start_dma = adc_start_dma,
};


static void hal_adc_start_calibration(xADC_TypeDef *adc)
{
    adc->CR2 |= (1 << 3);   /* RSTCAL */
    while (adc->CR2 & (1 << 3));
    adc->CR2 |= (1 << 2);   /* CAL */
    while (adc->CR2 & (1 << 2));
}
// 构造函数实现
Adc* adc_create(const device_info_t *info) {
    Adc* obj = (Adc*)os_malloc(sizeof(Adc));
    if (obj) {
        memset(obj, 0, sizeof(Adc));
        adc_init(obj, info);
    }
    return obj;
}

void adc_init(Adc* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(adc_fun);
    // TODO: 初始化派生类特有成员
	def_dev_init(self) = adc_dev_init_impl;
	def_dev_read(self) = adc_dev_read_impl;
	def_dev_write(self) = adc_dev_write_impl;
	def_dev_ioctl(self) = adc_dev_ioctl_impl;
}

void adc_deinit(Adc* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void adc_destroy(Adc* self) {
    if (self != NULL) {
        adc_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(adc_dev_init_impl) {
    // TODO: add dev_init method
    Adc *adc = (Adc *)self;
    const adc_config_t *conf = self->info->conf;
    //params
    LOG_DEBUG("adc", "adc init");
    if (conf->id >= ADC_MAX) {
        return;
    }

    /* 1. 通过 PinMux 将所有引脚设为模拟模式 */
    for (int i = 0; i < conf->num_channels; i++) {
        pin_config_t pin_cfg = {
                .port   = conf->channels[i]->port,
                .pin    = conf->channels[i]->pin,
                .mode   = PIN_MODE_ANALOG,
                .otype  = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_LOW,
                .pupd   = PIN_PUPD_NONE,
                .irq_mode = PIN_IRQ_MODE_NONE,
        };
        if (pinmux_request(&pin_cfg) != PINMUX_SUCCESS) {
            LOG_ERROR("adc", "channel pin pinmux error");
            return;
        }
    }

    /* 2. 使能时钟 */
    hal_adc_clock_enable(conf->id);

    xADC_TypeDef *adc_ctrl = ADCx[conf->id];

    /* 3. 配置分辨率和对齐方式 */
    uint32_t cr1 = adc_ctrl->CR1;
    cr1 &= ~(3 << 24);   /* RES[1:0] */
    cr1 |= (conf->resolution & 0x3) << 24;
    if (conf->align == ADC_ALIGN_LEFT) {
        cr1 |= (1 << 11);
    } else {
        cr1 &= ~(1 << 11);
    }
    adc_ctrl->CR1 = cr1;

    /* 4. 配置采样时间（SMPR1/SMPR2） */
    for (int i = 0; i < conf->num_channels; i++) {
        uint8_t ch = conf->channels[i]->channel;
        uint8_t smp = conf->channels[i]->sample_time;
        if (ch < 10) {
            adc_ctrl->SMPR2 &= ~(7 << (ch * 3));
            adc_ctrl->SMPR2 |= (smp & 0x7) << (ch * 3);
        } else {
            ch -= 10;
            adc_ctrl->SMPR1 &= ~(7 << (ch * 3));
            adc_ctrl->SMPR1 |= (smp & 0x7) << (ch * 3);
        }
    }

    /* 5. 配置规则通道序列 */
    uint32_t sqr1 = (conf->num_channels - 1) << 20;  /* L[3:0] */
    uint32_t sqr2 = 0, sqr3 = 0;
    for (int i = 0; i < conf->num_channels; i++) {
        uint8_t ch = conf->channels[i]->channel;
        if (i < 6) {
            sqr3 |= (ch & 0x1F) << (i * 5);
        } else if (i < 12) {
            sqr2 |= (ch & 0x1F) << ((i - 6) * 5);
        } else {
            sqr1 |= (ch & 0x1F) << ((i - 12) * 5);
        }
    }
    adc_ctrl->SQR1 = sqr1;
    adc_ctrl->SQR2 = sqr2;
    adc_ctrl->SQR3 = sqr3;

    /* 6. 使能 ADC */
    adc_ctrl->CR2 |= (1 << 0);   /* ADON */
    /* 等待稳定 (参考手册: tSTAB) */
    for (volatile uint32_t i=0; i<100000; i++);
    /* 7. 校准 */
    hal_adc_start_calibration(adc_ctrl);
    /* 8. DMA 可选 */
    if (conf->dma_cfg) {
        if (dma_stream_request(conf->dma_cfg) != 0) {
            return ;
        }
        //adc_dma_cfgs[cfg->id] = *conf->dma_cfg;
        //adc_dma_used[cfg->id] = true;

        /* 使能 DMA (CR2 bit8) 且如果是连续模式则置位 CONT (CR2 bit1) */
        adc_ctrl->CR2 |= (1 << 8);   /* DMA */
        if (conf->continuous) {
            adc_ctrl->CR2 |= (1 << 1);   /* CONT */
        }
        /* DDS (CR2 bit9) 通常不用设置（每次转换完发出请求） */

        if (conf->dma_cfg->it_enable) {
            self->irq_conf->handler = adc_txdma_irq_handler_impl;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg));
            self->fun->config_irq(self, self->irq_conf);
        }
    }
}
// dev_read method
dev_read_override(adc_dev_read_impl) {
    // TODO: add dev_read method
    Adc *adc = (Adc *)self;
    const adc_config_t *conf = self->info->conf;
    //params , void *buf, size_t count
    if (conf->id >= ADC_MAX) {
        return 0;
    }
    xADC_TypeDef *adc_ctrl = ADCx[conf->id];
    /* 启动转换 */
    adc_ctrl->CR2 |= (1 << 30);  /* SWSTART */
    /* 等待转换结束 */
    while (!(adc_ctrl->SR & (1 << 1)));  /* EOC */
    *(uint16_t *)buf = (uint16_t)adc_ctrl->DR;
    return 0;
}


// start_dma method
static int adc_start_dma(Adc* self, uint16_t *buffer, uint16_t count) {
    // TODO: add start_dma method
    const adc_config_t *conf = GET_DEVICE(self)->info->conf;
    if (conf->id >= ADC_MAX) {
        return ADC_ERROR;
    }

    /* 确保 DMA 流空闲 */
    if (dma_is_busy(conf->dma_cfg)) {
        return ADC_ERROR;
    }
    /* 启动 DMA 传输：外设地址 = &ADCx->DR, 存储器地址 = buffer, 方向 P2M */
    uint32_t per_addr = (uint32_t)&ADCx[conf->id]->DR;
    return dma_start_transfer(conf->dma_cfg, per_addr, (uint32_t)buffer, count);
}

static bool adc_txdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Adc *adc = (Adc *)irq_conf->arg;
    const adc_config_t *conf = GET_DEVICE(adc)->info->conf;
    dma_clear_flag(conf->dma_cfg);
    return true;
}

// dev_write method
dev_write_override(adc_dev_write_impl) {
    // TODO: add dev_write method
    Adc *adc = (Adc *)self;
    //params , const void *buf, size_t count
    
}
// dev_ioctl method
dev_ioctl_override(adc_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Adc *adc = (Adc *)self;
    //params , int cmd, void *arg
    
}


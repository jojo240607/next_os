#include "sdio.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "common/rcc.h"
#include "hal/hal_sdio.h"

dev_init_override(sdio_dev_init_impl);

// 析构函数声明
static void sdio_destroy(Sdio* self);
static bool sdio_irq_handler_impl(void *arg);

// TODO: 初始化数据成员
static const SdioFun sdio_fun = {
    .destroy = sdio_destroy,
};




// 构造函数实现
Sdio* sdio_create(const sdio_config_t *conf, const dev_pripority_t *priority) {
    Sdio* obj = (Sdio*)os_malloc(sizeof(Sdio));
    if (obj) {
        memset(obj, 0, sizeof(Sdio));
        sdio_init(obj, conf, priority);
    }
    return obj;
}

void sdio_init(Sdio* self, const sdio_config_t *conf, const dev_pripority_t *priority) {
    // 初始化基类部分
    device_init(&self->base, priority);
    self->fun = &(sdio_fun);
    // TODO: 初始化派生类特有成员
    self->conf = conf;
	def_dev_init(self) = sdio_dev_init_impl;
}
static void sdio_clock_config(const sdio_config_t *cfg)
{
    uint32_t clkcr = 0;

    /* 时钟边沿 */
    if (cfg->clock_edge == SDIO_CLOCK_EDGE_FALLING)
        clkcr |= (1UL << 12);  /* NEGEDGE */

    /* 省电模式 */
    if (cfg->power_save == SDIO_POWERSAVE_ENABLE)
        clkcr |= (1UL << 11);  /* PWRSAV */

    /* 硬件流控 */
    if (cfg->hw_flow_control == SDIO_FLOW_ENABLE)
        clkcr |= (1UL << 14);  /* HWFC_EN */

    /* 总线宽度 */
    if (cfg->bus_width == SDIO_BUS_WIDTH_4)
        clkcr |= (1UL << 10);  /* WIDBUS_4 */

    /* 分频系数 */
    clkcr |= (cfg->clock_div & 0xFF) << 0;

    /* 时钟使能 (使用分频后的时钟，旁路关闭) */
    clkcr |= (1UL << 8);   /* CLKEN */

    xSDIO->CLKCR = clkcr;
}

static void sdio_dma_enable(const sdio_config_t *cfg)
{
    if (cfg->dma_cfg && cfg->dma_cfg->tx_dma) {
        dma_stream_request(cfg->dma_cfg->tx_dma);
        //sdio_tx_dma = *cfg->dma_cfg->tx_dma;
        //sdio_dma_tx_used = true;
    }
    if (cfg->dma_cfg && cfg->dma_cfg->rx_dma) {
        dma_stream_request(cfg->dma_cfg->rx_dma);
        //sdio_rx_dma = *cfg->rx_dma_cfg;
        //sdio_dma_rx_used = true;
    }
    /* 使能 SDIO 的 DMA 请求: DCTRL 位3 */
    xSDIO->DCTRL |= (1UL << 3);   /* DMAEN */
}


/* =========================================================================
   硬件抽象层：引脚初始化
   ========================================================================= */
static int sdio_pins_init(const sdio_pins_t *pins)
{
    const pin_config_t pin_cfgs[] = {
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE, .af = pins->clk_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = pins->cmd_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = pins->d0_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = pins->d1_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = pins->d2_pin },
            { .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP, .af = pins->d3_pin },
    };
    return pinmux_request_group(pin_cfgs, 6);
}

/* =========================================================================
   命令发送核心函数（阻塞版）
   ========================================================================= */
int sdio_send_cmd(sdio_cmd_t *cmd)
{
    uint32_t timeout = 0xFFFFF;

    /* 等待硬件空闲 */
    while ((xSDIO->STA & (xSDIO_STA_CMDSENT | xSDIO_STA_CMDREND)) == 0) {
        if (--timeout == 0) {
            cmd->error = -1;
            return -1;
        }
    }

    /* 清除上一次的标志 */
    xSDIO->ICR = 0xFFFFFFFF;

    /* 写参数 */
    xSDIO->ARG = cmd->arg;

    /* 构造命令 */
    uint32_t reg = cmd->cmd & 0x3F;
    switch (cmd->resp_type) {
        case SDIO_RESPONSE_NO:
            reg |= xSDIO_CMD_WAITRESP_NO;
            break;
        case SDIO_RESPONSE_SHORT:
            reg |= xSDIO_CMD_WAITRESP_SHORT;
            break;
        case SDIO_RESPONSE_LONG:
            reg |= xSDIO_CMD_WAITRESP_LONG;
            break;
        case SDIO_RESPONSE_SHORT_NO_CRC:
            reg |= xSDIO_CMD_WAITRESP_SHORT;
            break;
    }
    reg |= xSDIO_CMD_CPSMEN;  /* 启动命令状态机 */

    xSDIO->CMD = reg;

    /* 等待完成或超时 */
    timeout = 0xFFFFF;
    while (1) {
        uint32_t sta = xSDIO->STA;
        if (sta & (xSDIO_STA_CCRCFAIL | xSDIO_STA_CTIMEOUT)) {
            cmd->error = -1;
            return -1;
        }
        if ((sta & xSDIO_STA_CMDREND) || (sta & xSDIO_STA_CMDSENT)) {
            break;
        }
        if (--timeout == 0) {
            cmd->error = -1;
            return -1;
        }
    }

    /* 读取响应 */
    if (cmd->resp_type != SDIO_RESPONSE_NO) {
        cmd->resp[0] = xSDIO->RESP[0];
        if (cmd->resp_type == SDIO_RESPONSE_LONG) {
            cmd->resp[1] = xSDIO->RESP[1];
            cmd->resp[2] = xSDIO->RESP[2];
            cmd->resp[3] = xSDIO->RESP[3];
        }
    }

    cmd->error = 0;
    return 0;
}


/* =========================================================================
   数据搬运（DMA + FIFO）
   ========================================================================= */
static int sdio_data_transfer(Sdio* self, sdio_data_t *data)
{
    uint32_t i, timeout;

    if (self->conf->dma_cfg && self->conf->dma_cfg->tx_dma && data->dir_to_card) {
        /* TX DMA: 存储器到外设 */
        dma_start_transfer(self->conf->dma_cfg->tx_dma, (uint32_t)data->buf, (uint32_t)&xSDIO->FIFO,
                           data->len / 4);
    } else if (self->conf->dma_cfg && self->conf->dma_cfg->rx_dma && !data->dir_to_card) {
        /* RX DMA: 外设到存储器 */
        dma_start_transfer(self->conf->dma_cfg->rx_dma, (uint32_t)&xSDIO->FIFO, (uint32_t)data->buf,
                           data->len / 4);
    } else {
        /* 轮询 FIFO */
        uint32_t words = data->len / 4;
        if (data->dir_to_card) {
            for (i = 0; i < words; i++) {
                timeout = 0xFFFFF;
                while ((xSDIO->STA & (1UL << 11)) == 0) {  /* TXFIFOHE */
                    if (--timeout == 0) {
                        return -1;
                    }
                }
                xSDIO->FIFO = ((uint32_t *)data->buf)[i];
            }
        } else {
            for (i = 0; i < words; i++) {
                timeout = 0xFFFFF;
                while ((xSDIO->STA & (1UL << 12)) == 0) {  /* RXFIFOHF */
                    if (--timeout == 0) {
                        return -1;
                    }
                }
                ((uint32_t *)data->buf)[i] = xSDIO->FIFO;
            }
        }
    }

    /* 等待 DATAEND */
    timeout = 0xFFFFFF;
    while (!(xSDIO->STA & xSDIO_STA_DATAEND)) {
        if (--timeout == 0) {
            return -1;
        }
    }
    data->error = 0;
    return 0;
}

/* =========================================================================
   命令 + 数据传输
   ========================================================================= */
int sdio_transfer_data(Sdio* self, sdio_cmd_t *cmd, sdio_data_t *data)
{
    uint32_t timeout;

    /* 初始化数据传输 */
    xSDIO->DLEN   = data->len;
    xSDIO->DTIMER = 0xFFFFFFFF;  /* 最大超时 */
    uint32_t dctrl = (data->block_size << 4);  /* DBLOCKSIZE */
    if (data->dir_to_card)
        dctrl |= (1UL << 0);  /* DTDIR: 控制器到卡 */
    dctrl |= (1UL << 1);      /* DTEN: 启动数据传输 */
    xSDIO->DCTRL = dctrl;

    /* 发送命令 */
    int ret = sdio_send_cmd(cmd);
    if (ret < 0) {
        return ret;
    }

    /* 执行数据搬运 */
    return sdio_data_transfer(self, data);
}

void sdio_deinit(Sdio* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void sdio_destroy(Sdio* self) {
    if (self != NULL) {
        sdio_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(sdio_dev_init_impl) {
    // TODO: add dev_init method
    Sdio *sdio = (Sdio *)self;
    //params 
    if (!sdio->conf) {
        return ;
    }

    hal_sdio_clock_enable();

    /* 2. 初始化引脚 (AF = SDIO = 12) */
    if (sdio_pins_init(&sdio->conf->pins) != 0) {
        return ;
    }

    /* 3. SDIO 上电 */
    xSDIO->POWER = xSDIO_POWER_PWRCTRL_ON;
    for (volatile int i = 0; i < 10000; i++);  /* 延时等待 */
    /* 4. 时钟配置 */
    sdio_clock_config(sdio->conf);
    /* 5. 中断配置 */
    //sdio_interrupt_config(sdio->conf);

    /* 配置中断屏蔽 */
    xSDIO->MASK = sdio->conf->it_enable;

    /* 使能 NVIC: SDIO 中断号 = 49 (SDIO_IRQn) */
    if (sdio->conf->it_enable) {
        // nvic_set_priority(SDIO_IRQn, 1, 0);
        // nvic_enable_irq(SDIO_IRQn);
        //self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
        self->irq_conf.handler = sdio_irq_handler_impl;
        self->irq_conf.semaphore = sem;
        self->irq_conf.arg = self;
        self->irq_conf.irq_num = SDIO_IRQ;
        self->fun->attach_irq(self, &self->irq_conf);
    }

    /* 6. DMA 配置 */
    sdio_dma_enable(sdio->conf);

}

static bool sdio_irq_handler_impl(void *arg) {
    uint32_t sta = xSDIO->STA;

    if (sta & SDIO_IT_CTIMEOUT) {
        /* 命令超时 */
    }
    if (sta & SDIO_IT_CCRCFAIL) {
        /* 命令CRC失败 */
    }
    if (sta & SDIO_IT_DTIMEOUT) {
        /* 数据超时 */
    }
    if (sta & SDIO_IT_DCRCFAIL) {
        /* 数据CRC失败 */
    }

    /* 清除中断标志 */
    xSDIO->ICR = sta;
}


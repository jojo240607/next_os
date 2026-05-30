#include "spi.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_spi.h"

static void spi_transfer_it(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);

static void spi_transfer_dma(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);

static void spi_transfer(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);
static bool spi_irq_handler_impl(nvic_irq_t *irq_conf);
static bool spi_txdma_irq_handler_impl(nvic_irq_t *irq_conf);
static bool spi_rxdma_irq_handler_impl(nvic_irq_t *irq_conf);

dev_init_override(spi_dev_init_impl);


// 析构函数声明
static void spi_destroy(Spi* self);

// TODO: 初始化数据成员
static const SpiFun spi_fun = {
    .destroy = spi_destroy,
	.transfer = spi_transfer,
	.transfer_dma = spi_transfer_dma,
	.transfer_it = spi_transfer_it,
};

// 构造函数实现
Spi* spi_create(const device_info_t *info) {
    Spi* obj = (Spi*)os_malloc(sizeof(Spi));
    if (obj) {
        memset(obj, 0, sizeof(Spi));
        spi_init(obj, info);
    }
    return obj;
}

void spi_init(Spi* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(spi_fun);
    // TODO: 初始化派生类特有成员
	def_dev_init(self) = spi_dev_init_impl;
    self->spi_xfer = NULL;

}


void spi_deinit(Spi* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void spi_destroy(Spi* self) {
    if (self != NULL) {
        spi_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(spi_dev_init_impl) {
    LOG_DEBUG("spi", "spi init");
    // TODO: add dev_init method
    Spi *spi = (Spi *)self;
    const spi_config_t *conf = self->info->conf;
    //params 
    if (conf->id >= SPI_MAX)
        return;

    /* 1. 通过 PinMux 申请引脚 (AF5) */
    const spi_pins_t *p = &conf->pins;
    /*
     *  SPI接口	常用默认引脚	AF配置 (复用功能)	备注
        SPI1	PA5-PA7, PA4(NSS)	AF5	我们在示例中使用过
        SPI2	PB12-PB15	AF5	我们在示例中使用过
        SPI3	PA15, PC10-PC12 等	AF6 (或AF5，取决于引脚)	取决于具体引脚，常见为AF6
        SPI4	PE2-PE6, PE14 等	AF5 (或AF6，取决于引脚)	取决于具体引脚，常见为AF5
        SPI5	PF7-PF9, PF6(NSS)	AF5 (或AF6，取决于引脚)	取决于具体引脚，常见为AF5
        SPI6	PG12-PG14, PG8(NSS)	AF5	通常为AF5 */
    pin_config_t pins[] = {
            {
                .mode = PIN_MODE_AF,
                .otype = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_HIGH,
                .pupd = PIN_PUPD_NONE,
                .af = p->sck_pin },
            {
                .mode = PIN_MODE_AF,
                .otype = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_HIGH,
                .pupd = PIN_PUPD_NONE,
                .af = p->mosi_pin },
            {
                .mode = PIN_MODE_AF,
                .otype = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_HIGH,
                .pupd = PIN_PUPD_NONE,
                .af = p->miso_pin },
            {
                .mode = PIN_MODE_AF,
                .otype = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_HIGH,
                .pupd = PIN_PUPD_NONE,
                .af = p->nss_pin }  // 可选，硬件NSS
    };
    for (int i = 0; i < 4; i++) {
        if (pinmux_request(&pins[i]) != PINMUX_SUCCESS) {
            return;
        }
    }

    /* 2. 使能时钟 */
    hal_spi_clock_enable(conf->id);

    /* 3. 配置 SPI */
    hal_spi_init(conf->id, conf->mode, conf->frame_format, conf->baudrate_div, conf->master);

    /* CR2 */
    hal_spi_disable_it(conf->id);
    if (hal_spi_it_init(conf->id, conf->it_enable)) {
        self->irq_conf->handler = spi_irq_handler_impl;
        self->irq_conf->arg = self;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, SPI1_IRQ + conf->id);
        self->fun->config_irq(self, self->irq_conf);
        if (!spi->spi_xfer) {
            spi->spi_xfer = os_malloc(sizeof(spi_xfer_t));
            memset(spi->spi_xfer, 0, sizeof(spi_xfer_t));
            spi->spi_xfer->spi_rx_sem = semaphore_create(0);
            spi->spi_xfer->spi_tx_sem = semaphore_create(0);
        }
    }
    /* 4. 申请 DMA 流 (可选) */
    if (conf->dma_cfg) {
        if (conf->dma_cfg->tx_dma) {
            if (dma_stream_request(conf->dma_cfg->tx_dma) != DMA_SUCCESS) {
                return;
            }
            if (conf->dma_cfg->tx_dma->it_enable) {
                self->irq_conf->handler = spi_txdma_irq_handler_impl;
                self->irq_conf->arg = self;
                self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg->tx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
        }
        if (conf->dma_cfg->rx_dma) {
            if (dma_stream_request(conf->dma_cfg->rx_dma) != DMA_SUCCESS) {
                return;
            }
            if (conf->dma_cfg->rx_dma->it_enable) {
                self->irq_conf->handler = spi_rxdma_irq_handler_impl;
                self->irq_conf->arg = self;
                self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg->rx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
        }
        hal_spi_dma_init(conf->id, conf->dma_cfg->tx_dma, conf->dma_cfg->rx_dma);

    }
}


// transfer method
static void spi_transfer(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len) {
    // TODO: add transfer method
    const spi_config_t *conf = GET_DEVICE(self)->info->conf;
    if (conf->id >= SPI_MAX) {
        return;
    }

    uint32_t sr = hal_spi_get_it_event(conf->id);
    for (int i = 0; i < len; i++) {
        while (!(sr & xSPI_IT_TXE));      // 等待发送缓冲区空
        uint8_t tx_byte = tx_data ? tx_data[i] : 0xFF;   // 发送数据或哑字节
        *hal_spi_data_addr(conf->id) = tx_byte;
        while (!(sr & xSPI_IT_RXNE));     // 等待接收缓冲区非空
        uint8_t rx_byte = *hal_spi_data_addr(conf->id);
        if (rx_data) {
            rx_data[i] = rx_byte;
        }
    }
    return;
}


// transfer_dma method
static void spi_transfer_dma(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len) {
    const spi_config_t *conf = GET_DEVICE(self)->info->conf;
    if (conf->id >= SPI_MAX) {
        return;
    }

    /* TX DMA */
    if (conf->dma_cfg && conf->dma_cfg->tx_dma && tx_data) {
        if (dma_is_busy(conf->dma_cfg->tx_dma)) {
            return ;
        }
        dma_start_transfer(conf->dma_cfg->tx_dma, (uint32_t)tx_data, (uint32_t)hal_spi_data_addr(conf->id), len);
        //dma_tx_busy[id] = true;
    }

    /* RX DMA */
    if (conf->dma_cfg  && conf->dma_cfg->rx_dma && rx_data) {
        if (dma_is_busy(conf->dma_cfg->rx_dma)) {
            return;
        }
        dma_start_transfer(conf->dma_cfg->rx_dma, (uint32_t)hal_spi_data_addr(conf->id), (uint32_t)rx_data, len);
        //dma_rx_busy[id] = true;
    }
    return ;
}

static bool spi_irq_handler_impl(nvic_irq_t *irq_conf) {
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    uint16_t sr = hal_spi_get_it_event(conf->id);

    // --- 错误处理 (优先) ---
    if (sr & (0x07 << 4)) {   // OVR, MODF, CRCERR, etc.
        // 清除错误标志：根据手册，读 SR 后写 DR 可清除某些标志，或直接关 SPE 再开
        uint8_t dummy __attribute__((unused)) = *hal_spi_data_addr(conf->id);
        (void)dummy;
        hal_spi_disable(conf->id); // 临时关 SPE
        hal_spi_enable(conf->id);

        x->active = false;

        hal_spi_clear_it(conf->id, xSPI_IE_ERR | xSPI_IE_TXE | xSPI_IE_RXNE);// 关中断
        return true;
    }

    // --- 接收中断 (RXNE) ---
    if (sr & xSPI_IT_RXNE) {  // RXNE
        uint8_t data = *hal_spi_data_addr(conf->id);   // 读 DR 清 RXNE
        if (x->rx_buf && x->rx_index < x->total_len) {
            x->rx_buf[x->rx_index] = data;
        }
        x->rx_index++;   // 无论是否有缓冲区，都要计数
    }

    // --- 发送中断 (TXE) ---
    if (sr & xSPI_IT_TXE) {  // TXE
        if (x->tx_index < x->total_len) {
            uint8_t byte = x->tx_buf ? x->tx_buf[x->tx_index] : 0xFF;
            *hal_spi_data_addr(conf->id) = byte;
            x->tx_index++;
        } else {
            // 数据已发完，关闭 TXE 中断
            hal_spi_clear_it(conf->id, xSPI_IE_TXE);
        }
    }

    // --- 传输完成检查 ---
    // 当发送和接收都完成（索引达到长度）时，关闭中断并通知
    if (x->tx_index >= x->total_len && x->rx_index >= x->total_len && x->active) {
        x->active = false;
        hal_spi_clear_it(conf->id, xSPI_IE_RXNE | xSPI_IE_TXE);// 关闭 TXE/RXNE 中断（ERRIE 可保留或关）
        x->spi_tx_sem->fun->give(x->spi_tx_sem);
    }
    return true;
}

static bool spi_txdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    return true;
}

static bool spi_rxdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    dma_clear_flag(conf->dma_cfg->rx_dma);
    return true;
}



// transfer_it method
static void spi_transfer_it(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len) {
    const spi_config_t *conf = GET_DEVICE(self)->info->conf;
    if (conf->id >= SPI_MAX || len == 0) {
        LOG_DEBUG("spi", "spi id error");
        return;
    }
    spi_xfer_t *x = self->spi_xfer;
    if (x->active) {
        LOG_DEBUG("spi", "spi is active");
        return;   // 上一次传输未结束
    }

    x->tx_buf = tx_data;
    x->rx_buf = rx_data;
    x->total_len = len;
    x->tx_index = 0;
    x->rx_index = 0;
    x->active = true;
    hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);// TXEIE  RXNEIE

    // 启动传输：写入第一个字节，这将产生时钟并置 TXE 为低
    // 如果是全双工，即使 rx_data 为 NULL 也要发哑字节；如果 tx_data 为 NULL 也要发哑字节（0xFF）
    uint8_t first_byte = tx_data ? tx_data[0] : 0xFF;
    *hal_spi_data_addr(conf->id) = first_byte;
    x->tx_index = 1;
    LOG_DEBUG("spi", "spi sem take");
    x->spi_tx_sem->fun->take(x->spi_tx_sem);
    LOG_DEBUG("spi", "spi recv");
}


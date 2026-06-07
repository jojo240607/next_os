/**
 * SPI 驱动 — Device VTable + SVC 模式
 *
 * 用户接口（通过 SVC 调用）:
 *   dev_write(spi, tx_buf, count) → SPI 全双工传输，RX→内部 cache
 *   dev_read(spi, rx_buf, count)  → 从内部 cache 取最近一次传输的 RX 数据
 *   dev_ioctl(spi, SPI_IOCTL_TRANSFER, &args) → 直接指定 tx+rx 的传输
 *
 * 内部模式选择:
 *   dma_cfg != NULL → DMA 模式
 *   dma_cfg == NULL && it_enable → 中断模式
 *   否则 → 阻塞轮询
 */
#include "spi.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "hal/hal_spi.h"

/* ── 前向声明 ── */
static bool spi_irq_handler_impl(nvic_irq_t *irq_conf);
static bool spi_txdma_irq_handler_impl(nvic_irq_t *irq_conf);
static bool spi_rxdma_irq_handler_impl(nvic_irq_t *irq_conf);

/* ── Device VTable override 宏 ── */
dev_init_override(spi_dev_init_impl);
dev_read_override(spi_dev_read_impl);
dev_write_override(spi_dev_write_impl);
dev_ioctl_override(spi_dev_ioctl_impl);

static void spi_destroy(Spi* self);

/* ── 虚函数表 ── */
static const SpiFun spi_fun = {
    .destroy = spi_destroy,
};

/* ══════════════════════════════════════════════════════════════
   构造 / 析构
   ══════════════════════════════════════════════════════════════ */

Spi* spi_create(const device_info_t *info)
{
    Spi* obj = (Spi*)os_malloc(sizeof(Spi));
    if (obj) {
        memset(obj, 0, sizeof(Spi));
        spi_init(obj, info);
    }
    return obj;
}

void spi_init(Spi* self, const device_info_t *info)
{
    device_init(&self->base, info);
    self->fun = &spi_fun;

    GET_DEVICE_VTABLE(self)->dev_init  = spi_dev_init_impl;
    GET_DEVICE_VTABLE(self)->dev_read  = spi_dev_read_impl;
    GET_DEVICE_VTABLE(self)->dev_write = spi_dev_write_impl;
    GET_DEVICE_VTABLE(self)->dev_ioctl = spi_dev_ioctl_impl;
    const spi_config_t *conf = info->conf;
//    self->rx_cache_buf = ringbuf_create(conf->cache_size);
    if (!self->spi_xfer) {
        self->spi_xfer = os_malloc(sizeof(spi_xfer_t));
        memset(self->spi_xfer, 0, sizeof(spi_xfer_t));
        self->spi_xfer->tx_user_buf = os_malloc(sizeof(spi_cache_t));
        memset(self->spi_xfer->tx_user_buf, 0, sizeof(spi_cache_t));
        self->spi_xfer->rx_user_buf = os_malloc(sizeof(spi_cache_t));
        memset(self->spi_xfer->rx_user_buf, 0, sizeof(spi_cache_t));
        self->spi_xfer->spi_sem = semaphore_create(0);
        //self->spi_xfer->uart_rx_sem = semaphore_create(0);
    }
    self->cs_pin = NULL;
}

void spi_deinit(Spi* self)
{
    device_deinit(GET_DEVICE(self));
}

static void spi_destroy(Spi* self)
{
    if (self) { spi_deinit(self); os_free(self); }
}

/* ══════════════════════════════════════════════════════════════
   dev_init — 硬件初始化
   ══════════════════════════════════════════════════════════════ */

dev_init_override(spi_dev_init_impl)
{
    Spi *spi = (Spi *)self;
    const spi_config_t *conf = self->info->conf;

    if (conf->id >= SPI_MAX) return;

    /* 1. PinMux AF5 */
    const spi_pins_t *p = &conf->pins;
    pin_config_t pins[] = {
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->sck_pin },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->mosi_pin },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = p->miso_pin },
        /* NSS: 硬件模式用 AF，软件模式用 GPIO output */
        { .port = p->cs_pin.port, .pin = p->cs_pin.pin,
          .mode = (conf->nss_mode == SPI_NSS_HARD) ? PIN_MODE_AF : PIN_MODE_OUTPUT,
          .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP,
          .af = (conf->nss_mode == SPI_NSS_HARD) ? p->nss_pin : 0 },
    };

    for (int i = 0; i < 4; i++) {
        if (pinmux_request(&pins[i]) != PINMUX_SUCCESS) return;
    }
    if (conf->nss_mode == SPI_NSS_SOFT) {
        spi->cs_pin = &p->cs_pin;//AF_REQ_GET_PORT(conf->pins.nss_pin);
        gpio_set(spi->cs_pin);
    }
    /* 2. 时钟 + 配置 */
    hal_spi_clock_enable(conf->id);
    hal_spi_init(conf->id, conf->mode, conf->frame_format,
                 conf->baudrate_div, conf->master, conf->first_bit,
                 conf->nss_mode);

    /* 3. 中断初始化 */
    //spi_xfer_t *x = NULL;
    hal_spi_disable_it(conf->id);
    /* 4. DMA 初始化 */
    if (conf->dma_cfg) {
        //if (!spi->spi_xfer) {
        //    spi->spi_xfer = os_malloc(sizeof(spi_xfer_t));
        //    memset(spi->spi_xfer, 0, sizeof(spi_xfer_t));
        //    spi->spi_xfer->spi_sem = semaphore_create(0);
        //}
        if (conf->dma_cfg->tx_dma) {
            dma_stream_request(conf->dma_cfg->tx_dma);
            if (conf->dma_cfg->tx_dma->it_enable) {
                self->irq_conf->handler = spi_txdma_irq_handler_impl;
                self->irq_conf->arg = self;
                self->irq_conf->irq_list->fun->add_int(
                    self->irq_conf->irq_list,
                    dma_get_irqnum(conf->dma_cfg->tx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
        }
        if (conf->dma_cfg->rx_dma) {
            dma_stream_request(conf->dma_cfg->rx_dma);
            if (conf->dma_cfg->rx_dma->it_enable) {
                self->irq_conf->handler = spi_rxdma_irq_handler_impl;
                self->irq_conf->arg = self;
                self->irq_conf->irq_list->fun->add_int(
                    self->irq_conf->irq_list,
                    dma_get_irqnum(conf->dma_cfg->rx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
        }
        hal_spi_dma_init(conf->id, conf->dma_cfg->tx_dma,
                         conf->dma_cfg->rx_dma);
    } else if (hal_spi_it_init(conf->id, conf->it_enable)) {
        self->irq_conf->handler = spi_irq_handler_impl;
        self->irq_conf->arg = self;
        self->irq_conf->irq_list->fun->add_int(
                self->irq_conf->irq_list, (int) (SPI1_IRQ + conf->id));
        self->fun->config_irq(self, self->irq_conf);
        /* spi_xfer 已在 spi_init 中创建，此处无需重复 */
    }

    LOG_DEBUG("spi", "spi%d init ok, mode=%s", conf->id,
              conf->dma_cfg ? "dma" : conf->it_enable ? "it" : "poll");
}

/* ══════════════════════════════════════════════════════════════
   dev_read — 从内部 RX cache 取数据
   ══════════════════════════════════════════════════════════════ */

dev_read_override(spi_dev_read_impl)
{
    Spi *spi = (Spi *)self;
    spi_xfer_t *x = spi->spi_xfer;
    const spi_config_t *conf = self->info->conf;
    if (conf->dma_cfg && conf->dma_cfg->tx_dma
        && conf->dma_cfg->rx_dma) {
        /* ── DMA 模式: 从 ringbuf 取 dev_write 时 DMA 写入的 RX 数据 ── */
        //ringbuf_get(spi->rx_cache_buf, (uint8_t *)buf, count);



    } else if (conf->it_enable) {
        if (!x || x->active) {
            return 0;
        }
        x->rx_user_buf->buf      = buf;
        x->rx_user_buf->buf_size = count;
        x->rx_user_buf->pos      = 0;

        x->tx_user_buf->buf      = NULL;
        x->tx_user_buf->buf_size = 0;
        x->tx_user_buf->pos      = 0;
        x->active                = true;
        hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);
        /* 写入第一个字节触发时钟 */
        uint8_t first = x->tx_user_buf->buf ? x->tx_user_buf->buf[0] : 0xFF;
        *hal_spi_data_addr(conf->id) = first;
        x->tx_user_buf->pos = 1;
        x->spi_sem->fun->take(x->spi_sem);
    } else {
        hal_spi_receive(conf->id, spi->cs_pin, buf, count);
    }
    return count;
}

/* ══════════════════════════════════════════════════════════════
   dev_write — SPI 全双工传输（IT / DMA / 阻塞）
   ══════════════════════════════════════════════════════════════ */

dev_write_override(spi_dev_write_impl)
{
    Spi *spi = (Spi *)self;
    const spi_config_t *conf = self->info->conf;

    if (conf->id >= SPI_MAX || count == 0) return;

    spi_xfer_t *x = spi->spi_xfer;

    if (conf->dma_cfg && conf->dma_cfg->tx_dma
        && conf->dma_cfg->rx_dma) {
        /* ─── DMA 模式 ─── */
        if (!x || dma_is_busy(conf->dma_cfg->tx_dma)) {
            return;
        }
        x->active = true;
        /* TX DMA: 内存 → SPI_DR (发送) */
        hal_spi_send_dma(conf->id, conf->dma_cfg->tx_dma, buf, count);
        /* RX DMA: SPI_DR → ringbuf buffer (接收) */
        //hal_spi_recv_dma(conf->id, conf->dma_cfg->rx_dma,
        //                 spi->rx_cache_buf->buffer, l);

        /* TX TC 中断唤醒 (RX 同步完成，无需单独等待) */
        x->spi_sem->fun->take(x->spi_sem);

    } else if (conf->it_enable) {
        /* ─── 中断模式 ─── */
        if (!x || x->active) {
            return;
        }
        x->rx_user_buf->buf      = NULL;
        x->rx_user_buf->buf_size = 0;
        x->rx_user_buf->pos      = 0;

        x->tx_user_buf->buf      = (uint8_t *)buf;
        x->tx_user_buf->buf_size = count;
        x->tx_user_buf->pos      = 0;
        x->active                = true;

        hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);

        /* 写入第一个字节触发时钟 */
        uint8_t first = x->tx_user_buf->buf ? x->tx_user_buf->buf[0] : 0xFF;
        *hal_spi_data_addr(conf->id) = first;
        x->tx_user_buf->pos = 1;
        x->spi_sem->fun->take(x->spi_sem);
    } else {
        /* ─── 阻塞轮询模式 ─── */
        hal_spi_transmit(conf->id, spi->cs_pin, buf, count);
    }
}

/* ══════════════════════════════════════════════════════════════
   dev_ioctl — 扩展控制（直接指定 tx+rx 的传输等）
   ══════════════════════════════════════════════════════════════ */

dev_ioctl_override(spi_dev_ioctl_impl)
{
    Spi *spi = (Spi *)self;
    const spi_config_t *conf = self->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    if (cmd == DEVICE_TRANSFER && arg) {
        spi_transfer_args_t *a = (spi_transfer_args_t *)arg;
        /* 先 dev_write 发送，再 dev_read 取回 */
        if (conf->dma_cfg && conf->dma_cfg->tx_dma
            && conf->dma_cfg->rx_dma) {
            if (spi->cs_pin) {
                gpio_reset(spi->cs_pin);
            } else {
                hal_spi_enable(conf->id);
            }
            x->active                = true;
            hal_spi_send_dma(conf->id, conf->dma_cfg->tx_dma, a->tx_buf, a->len);
            /* RX DMA: SPI_DR → ringbuf buffer (接收) */
            hal_spi_recv_dma(conf->id, conf->dma_cfg->rx_dma,
                             a->rx_buf, a->len);

        } else if (conf->it_enable) {
            if (spi->cs_pin) {
                gpio_reset(spi->cs_pin);
            } else {
                hal_spi_enable(conf->id);
            }
            x->rx_user_buf->buf = a->rx_buf;
            x->rx_user_buf->buf_size =  a->len;
            x->rx_user_buf->pos      = 0;

            x->tx_user_buf->buf      = (uint8_t *)a->tx_buf;
            x->tx_user_buf->buf_size =  a->len;
            x->tx_user_buf->pos      = 0;
            x->active                = true;

            hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);

            /* 写入第一个字节触发时钟 */
            uint8_t first = a->tx_buf ? a->tx_buf[0] : 0xFF;
            *hal_spi_data_addr(conf->id) = first;
            x->tx_user_buf->pos = 1;

            x->spi_sem->fun->take(x->spi_sem);

        } else {
            hal_spi_transfer(conf->id, spi->cs_pin, a->tx_buf, a->rx_buf, a->len);
        }
    }
}

/* ══════════════════════════════════════════════════════════════
   SPI 中断处理
   ══════════════════════════════════════════════════════════════ */

static bool spi_irq_handler_impl(nvic_irq_t *irq_conf)
{
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    uint16_t sr = hal_spi_get_it_event(conf->id);

    /* 错误处理 —— Renode 模拟可能产生假 MODF/OVR/CRCERR，只清不中断 */
    if (sr & (0x07 << 4)) {
        /* OVR: 读 DR 清除 (读走滞留数据，不影响传输) */
        if (sr & (1 << 6)) {
            volatile uint8_t dummy __attribute__((unused)) = *hal_spi_data_addr(conf->id);
            (void)dummy;
        }
        /* MODF: 写 CR1 清除 (不必关 SPE——手册: 读SR后写CR1即可清 MODF) */
        if (sr & (1 << 5)) {
            hal_spi_disable(conf->id);
            hal_spi_enable(conf->id);
            /* Note: 真机上 SSM=1 不会出 MODF，Renode 假信号忽略即可 */
        }
        /* CRCERR: CRC 未使能不应出现，可能是仿真假信号，忽略 */
        /* 不清除也不影响传输——这些标志本身不会停止 SPI 工作 */
    }

    /* RXNE */
    if (sr & xSPI_IT_RXNE) {
        uint8_t data = *hal_spi_data_addr(conf->id);
        if (x && x->rx_user_buf->buf) {
            x->rx_user_buf->buf[x->rx_user_buf->pos] = data;
            x->rx_user_buf->pos++;
        }
    }

    /* TXE */
    if (sr & xSPI_IT_TXE) {
        if (x && x->tx_user_buf->pos < x->tx_user_buf->buf_size) {
            uint8_t byte = x->tx_user_buf->buf ? x->tx_user_buf->buf[x->tx_user_buf->pos] : 0xFF;
            *hal_spi_data_addr(conf->id) = byte;
            x->tx_user_buf->pos++;
        } else {
            hal_spi_clear_it(conf->id, xSPI_IE_TXE);
        }
    }

    /* 传输完成 */
    if (x && x->tx_user_buf->pos >= x->tx_user_buf->buf_size && x->rx_user_buf->pos >= x->rx_user_buf->buf_size) {
        x->active = false;
        hal_spi_clear_it(conf->id, xSPI_IE_RXNE | xSPI_IE_TXE);
        if (spi->cs_pin) {
            gpio_set(spi->cs_pin);
        } else {
            hal_spi_disable(conf->id);
        }
        x->spi_sem->fun->give(x->spi_sem);
    }
    return true;
}

static bool spi_txdma_irq_handler_impl(nvic_irq_t *irq_conf)
{
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    if (spi->spi_xfer && spi->spi_xfer->active) {
        spi->spi_xfer->active = false;
        if (spi->cs_pin) {
            gpio_set(spi->cs_pin);
        } else {
            hal_spi_disable(conf->id);
        }
        spi->spi_xfer->spi_sem->fun->give(spi->spi_xfer->spi_sem);
    }
    return true;
}

static bool spi_rxdma_irq_handler_impl(nvic_irq_t *irq_conf)
{
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    dma_clear_flag(conf->dma_cfg->rx_dma);
    return true;
}

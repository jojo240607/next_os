/**
 * USART DMA 模式
 */
#include "usart.h"
#include "../hal/hal_usart.h"
#include "../common/dma.h"
#include "../../common/linear_pool.h"
//#include "../../common/nvic.h"

/* ── DMA 中断处理 ── */
static bool usart_txdma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    Usart *usart = GET_USART(dev);
    const usart_config_t *conf = dev->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    usart->uart_xfer->tx_user_active = false;
    dev->fun->trigger_event(dev, UART_TX_DONE, dev->arg);
    return true;
}

static bool usart_rxdma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    Usart *usart = GET_USART(dev);
    const usart_config_t *conf = dev->info->conf;
    uart_xfer_t *rx = usart->uart_xfer;
    dma_it_event_t it_event = dma_get_it_event(conf->dma_cfg->rx_dma);
    dma_clear_flag(conf->dma_cfg->rx_dma);
    volatile uint16_t cur_ndtr = uart_dma_get_rx_ndtr(conf->dma_cfg->rx_dma);
    size_t dma_pos = usart->rx_cache_buf->size - cur_ndtr;
    if (it_event == DMA_IT_EVENT_HTIF || it_event == DMA_IT_EVENT_TCIF) {
        if (rx->rx_user_buf->buf) {
            ringbuf_dma_update(usart->rx_cache_buf, dma_pos);
            size_t recv_size = ringbuf_get(usart->rx_cache_buf,
                rx->rx_user_buf->buf + rx->rx_user_buf->pos,
                rx->rx_user_buf->buf_size - rx->rx_user_buf->pos);
            rx->rx_user_buf->pos += recv_size;
        }
    }
    if (it_event == DMA_IT_EVENT_TCIF) {
        dev->fun->trigger_event(dev, UART_RX_DONE, dev->arg);
    }
    return true;
}

static bool usart_dma_idle_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    Usart *usart = GET_USART(dev);
    const usart_config_t *conf = dev->info->conf;
    uint32_t sr = hal_uart_get_it_event(conf->id);

    if (sr & xUART_FLAG_IDLE) {
        uart_xfer_t *rx = usart->uart_xfer;
        hal_uart_clear_idle_flag(conf->id, conf->dma_cfg->rx_dma);
        volatile uint16_t cur_ndtr = uart_dma_get_rx_ndtr(conf->dma_cfg->rx_dma);
        size_t dma_pos = usart->rx_cache_buf->size - cur_ndtr;
        ringbuf_dma_update(usart->rx_cache_buf, dma_pos);
        rx->rx_user_active = false;
        if (rx->rx_user_buf->buf) {
            size_t recv_size = ringbuf_get(usart->rx_cache_buf,
                rx->rx_user_buf->buf + rx->rx_user_buf->pos,
                rx->rx_user_buf->buf_size - rx->rx_user_buf->pos);
            rx->rx_user_buf->pos += recv_size;
            rx->rx_user_buf->buf = NULL;
            dev->fun->trigger_event(dev, UART_RX_DONE, dev->arg);
        }
    }
    if (sr & xUART_FLAG_ORE) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id); (void)dr;
    }
    return true;
}

/* ── dev_init ── */
void usart_dma_dev_init(Device *self) {
    Usart *usart = GET_USART(self);
    const usart_config_t *conf = self->info->conf;

    if (!usart->uart_xfer) {
        usart->uart_xfer = os_malloc(sizeof(uart_xfer_t));
        memset(usart->uart_xfer, 0, sizeof(uart_xfer_t));
        usart->uart_xfer->tx_user_buf = os_malloc(sizeof(uart_cache_t));
        memset(usart->uart_xfer->tx_user_buf, 0, sizeof(uart_cache_t));
        usart->uart_xfer->rx_user_buf = os_malloc(sizeof(uart_cache_t));
        memset(usart->uart_xfer->rx_user_buf, 0, sizeof(uart_cache_t));
    }

    if (conf->dma_cfg->tx_dma) {
        if (conf->dma_cfg->tx_dma->it_enable) {
            self->irq_conf->handler = usart_txdma_irq_handler;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
                dma_get_irqnum(conf->dma_cfg->tx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        dma_stream_request(conf->dma_cfg->tx_dma);
    }
    if (conf->dma_cfg->rx_dma) {
        if (conf->dma_cfg->rx_dma->it_enable) {
            self->irq_conf->handler = usart_rxdma_irq_handler;
            self->irq_conf->arg = self;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
                dma_get_irqnum(conf->dma_cfg->rx_dma));
            self->fun->config_irq(self, self->irq_conf);
        }
        dma_stream_request(conf->dma_cfg->rx_dma);
        if (usart->rx_cache_buf) {
            hal_uart_it_enable(conf->id, xUART_FLAG_IDLE);
            nvic_register(gloable_nvic, USART1_IRQ + conf->id,
                          usart_dma_idle_irq_handler, self, NULL);
        }
        hal_uart_dma_init(conf->id, conf->dma_cfg->tx_dma, conf->dma_cfg->rx_dma);
        uart_recv_dma(conf->id, conf->dma_cfg->rx_dma,
                      usart->rx_cache_buf->buffer, usart->rx_cache_buf->size);
    }
}

/* ── ioctl ── */
void usart_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

/* ── 读写 ── */
size_t usart_dma_read(Device *self, void *buf, size_t count) {
    Usart *usart = GET_USART(self);
    const usart_config_t *conf = self->info->conf;
    uart_xfer_t *x = usart->uart_xfer;

    size_t recv_size = ringbuf_get(usart->rx_cache_buf, (uint8_t *)buf, count);
    if (recv_size) return recv_size;

    x->rx_user_buf->buf      = (uint8_t *)buf;
    x->rx_user_buf->buf_size = (uint16_t)count;
    x->rx_user_buf->pos      = 0;
    x->rx_user_active        = true;
    self->fun->trigger_event(self, UART_RX_START, self->arg);
    return x->rx_user_buf->pos;
}

void usart_dma_write(Device *self, const void *buf, size_t count) {
    Usart *usart = GET_USART(self);
    const usart_config_t *conf = self->info->conf;
    uart_xfer_t *x = usart->uart_xfer;
    if (!x || x->tx_user_active) return;

    x->tx_user_active = true;
    uart_send_dma(conf->id, conf->dma_cfg->tx_dma, (uint8_t *)buf, (uint16_t)count);
    self->fun->trigger_event(self, UART_TX_START, self->arg);
}

/**
 * USART 中断模式
 */
#include "usart.h"
#include "../hal/hal_usart.h"
#include "../../common/linear_pool.h"

/* ── 中断处理 ── */
static bool usart_irq_handler(nvic_irq_t *irq_conf) {
    Usart *usart = (Usart *)irq_conf->arg;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    uart_xfer_t *x = usart->uart_xfer;
    uint32_t sr = hal_uart_get_it_event(conf->id);

    if (sr & xUART_FLAG_RXNE) {
        uint8_t data = *hal_uart_data_addr(conf->id);
        if (x) ringbuf_put(usart->rx_cache_buf, data);
    }

    if (sr & xUART_FLAG_IDLE) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id); (void)dr;
        if (x && usart->rx_cache_buf->count > 0) {
            hal_uart_it_disable(conf->id, xUART_FLAG_IDLE);
            x->rx_user_active = false;
            if (x->rx_user_buf->buf) {
                size_t recv_size = ringbuf_get(usart->rx_cache_buf,
                    x->rx_user_buf->buf, x->rx_user_buf->buf_size);
                x->rx_user_buf->pos = recv_size;
                x->rx_user_buf->buf = NULL;
                GET_DEVICE(usart)->fun->trigger_event(GET_DEVICE(usart), UART_RX_DONE, GET_DEVICE(usart)->arg);
            }
        }
    }

    if (sr & xUART_FLAG_TXE) {
        if (x && x->tx_user_active && x->tx_user_buf->pos < x->tx_user_buf->buf_size) {
            *hal_uart_data_addr(conf->id) = x->tx_user_buf->buf[x->tx_user_buf->pos++];
        } else {
            hal_uart_it_disable(conf->id, xUART_IT_TXE);
            if (x && x->tx_user_active) {
                x->tx_user_active = false;
                GET_DEVICE(usart)->fun->trigger_event(GET_DEVICE(usart), UART_TX_DONE, GET_DEVICE(usart)->arg);
            }
        }
    }

    if (sr & xUART_FLAG_TC) {
        hal_uart_it_clear(conf->id, xUART_IT_TC);
        if (x && x->tx_user_active && x->tx_user_buf->pos >= x->tx_user_buf->buf_size) {
            x->tx_user_active = false;
            GET_DEVICE(usart)->fun->trigger_event(GET_DEVICE(usart), UART_TX_DONE, GET_DEVICE(usart)->arg);
        }
    }

    if (sr & xUART_FLAG_ORE) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id); (void)dr;
        GET_DEVICE(usart)->fun->trigger_event(GET_DEVICE(usart), UART_XFER_ERROR, GET_DEVICE(usart)->arg);
    }
    if (sr & (xUART_FLAG_PE | xUART_FLAG_FE | xUART_FLAG_NE)) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id); (void)dr;
        GET_DEVICE(usart)->fun->trigger_event(GET_DEVICE(usart), UART_XFER_ERROR, GET_DEVICE(usart)->arg);
    }
    return true;
}

/* ── dev_init ── */
void usart_it_dev_init(Device *self) {
    const usart_config_t *conf = self->info->conf;
    Usart *usart = GET_USART(self);

    if (!usart->uart_xfer) {
        usart->uart_xfer = os_malloc(sizeof(uart_xfer_t));
        memset(usart->uart_xfer, 0, sizeof(uart_xfer_t));
        usart->uart_xfer->tx_user_buf = os_malloc(sizeof(uart_cache_t));
        memset(usart->uart_xfer->tx_user_buf, 0, sizeof(uart_cache_t));
        usart->uart_xfer->rx_user_buf = os_malloc(sizeof(uart_cache_t));
        memset(usart->uart_xfer->rx_user_buf, 0, sizeof(uart_cache_t));
    }

    hal_uart_it_init(conf->id, conf->it_enable);
    self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, USART1_IRQ + conf->id);
    self->irq_conf->handler = usart_irq_handler;
    self->irq_conf->arg = self;
    self->fun->config_irq(self, self->irq_conf);
}

/* ── ioctl ── */
void usart_it_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

/* ── 读写 ── */
size_t usart_it_read(Device *self, void *buf, size_t count) {
    Usart *usart = GET_USART(self);
    const usart_config_t *conf = self->info->conf;
    uart_xfer_t *x = usart->uart_xfer;
    if (!x) return 0;

    size_t recv_size = ringbuf_get(usart->rx_cache_buf, (uint8_t *)buf, count);
    if (recv_size) return recv_size;

    x->rx_user_buf->buf      = (uint8_t *)buf;
    x->rx_user_buf->buf_size = (uint16_t)count;
    x->rx_user_buf->pos      = 0;
    x->rx_user_active        = true;
    hal_uart_it_enable(conf->id, xUART_IT_RXNE | xUART_FLAG_IDLE);
    self->fun->trigger_event(self, UART_RX_START, self->arg);
    return x->rx_user_buf->pos;
}

void usart_it_write(Device *self, const void *buf, size_t count) {
    Usart *usart = GET_USART(self);
    const usart_config_t *conf = self->info->conf;
    uart_xfer_t *x = usart->uart_xfer;
    if (!x || x->tx_user_active) return;

    x->tx_user_buf->buf      = (uint8_t *)buf;
    x->tx_user_buf->buf_size = (uint16_t)count;
    x->tx_user_buf->pos      = 0;
    x->tx_user_active        = true;

    hal_uart_it_enable(conf->id, xUART_IT_TXE | xUART_IT_TC);
    self->fun->trigger_event(self, UART_TX_START, self->arg);
}

/**
 * USART 驱动 — 公共部分 (构造、硬件初始化)
 */
#include "usart.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../common/rcc.h"
#include "../hal/hal_usart.h"

/* ── 模式文件外部声明 ── */
extern size_t usart_poll_read(Device*, void*, size_t);
extern void   usart_poll_write(Device*, const void*, size_t);
extern void   usart_poll_ioctl(Device*, ioctl_cmd_t, void*);
extern void   usart_poll_dev_init(Device*);
extern size_t usart_it_read(Device*, void*, size_t);
extern void   usart_it_write(Device*, const void*, size_t);
extern void   usart_it_ioctl(Device*, ioctl_cmd_t, void*);
extern void   usart_it_dev_init(Device*);
extern size_t usart_dma_read(Device*, void*, size_t);
extern void   usart_dma_write(Device*, const void*, size_t);
extern void   usart_dma_ioctl(Device*, ioctl_cmd_t, void*);
extern void   usart_dma_dev_init(Device*);

static void usart_hw_init(Usart *self);
static void usart_destroy(Usart* self);
static const UsartFun usart_fun = { .destroy = usart_destroy };

/* ════ 构造 / 析构 ════ */
Usart* usart_create(const device_info_t *info) {
    if (!info) return NULL;
    Usart* obj = (Usart*)os_malloc(sizeof(Usart));
    if (obj) { memset(obj, 0, sizeof(Usart)); usart_init(obj, info); }
    return obj;
}

void usart_init(Usart* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &usart_fun;

    const usart_config_t *conf = (const usart_config_t *)info->conf;
    if (conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = usart_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = usart_dma_write;
        GET_DEVICE_VTABLE(self)->dev_read  = usart_dma_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = usart_dma_ioctl;
    } else if (conf->it_enable) {
        GET_DEVICE_VTABLE(self)->dev_init  = usart_it_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = usart_it_write;
        GET_DEVICE_VTABLE(self)->dev_read  = usart_it_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = usart_it_ioctl;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = usart_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_write = usart_poll_write;
        GET_DEVICE_VTABLE(self)->dev_read  = usart_poll_read;
        GET_DEVICE_VTABLE(self)->dev_ioctl = usart_poll_ioctl;
    }

    self->rx_cache_buf = ringbuf_create(conf->cache_size);
    self->uart_xfer    = NULL;
    self->uart_tx_sem  = semaphore_create(0);
    self->uart_rx_sem  = semaphore_create(0);

    /* 公共硬件初始化: PinMux + 时钟 + 使能, 立即执行 */
    usart_hw_init(self);
}

void usart_deinit(Usart* self) { device_deinit(GET_DEVICE(self)); }
static void usart_destroy(Usart* self) {
    if (self) { usart_deinit(self); os_free(self); }
}

/* ════ 内部 listener ════ */
static void default_usart_listener(Device *self, uint8_t event, void *arg) {
    Usart *usart = GET_USART(self);
    (void)arg;
    switch (event) {
        case UART_TX_START: usart->uart_tx_sem->fun->take(usart->uart_tx_sem); break;
        case UART_TX_DONE:  usart->uart_tx_sem->fun->give(usart->uart_tx_sem); break;
        case UART_RX_START: usart->uart_rx_sem->fun->take(usart->uart_rx_sem); break;
        case UART_RX_DONE:  usart->uart_rx_sem->fun->give(usart->uart_rx_sem); break;
        default: break;
    }
}

static void usart_hw_init(Usart *self) {
    Device *dev = GET_DEVICE(self);
    const usart_config_t *conf = dev->info->conf;
    if (conf->id >= UART_MAX) return;

    pin_config_t pins[2] = {
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = conf->pins.uart_tx },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP, .ospeed = PIN_OSPEED_HIGH,
         .pupd = PIN_PUPD_NONE, .af = conf->pins.uart_rx }
    };
    if (pinmux_request_group(pins, 2) != PINMUX_SUCCESS) {
        LOG_ERROR("usart", "pinmux error"); return;
    }

    hal_uart_clock_enable(conf->id);
    hal_uart_set_baudrate(conf->id, conf->baudrate);
    hal_uart_set_format(conf->id, conf->word_len, conf->stop_bits, conf->parity);
    hal_uart_enable(conf->id);

    dev->fun->register_listener(dev, default_usart_listener);
    LOG_DEBUG("usart", "usart%d hw init ok", conf->id);
}

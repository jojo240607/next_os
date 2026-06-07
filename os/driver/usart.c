#include "usart.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include "../log/log.h"
#include "common/rcc.h"
#include "hal/hal_usart.h"

static void usart_recv_it(Usart* self, uint8_t *buffer, uint16_t len);
static void usart_send_it(Usart* self, const uint8_t *data, uint16_t len);
static void usart_recv(Usart* self, uint8_t *buffer, uint16_t len);
static void usart_send(Usart* self, const uint8_t *data, uint16_t len);

dev_init_override(usart_dev_init_impl);
dev_read_override(usart_dev_read_impl);
dev_write_override(usart_dev_write_impl);
dev_ioctl_override(usart_dev_ioctl_impl);
static bool usart_irq_handler_impl(nvic_irq_t *irq_conf);
static bool usart_txdma_irq_handler_impl(nvic_irq_t *irq_conf);
static bool usart_rxdma_irq_handler_impl(nvic_irq_t *irq_conf);
static bool usart_dma_idle_irq_handler_impl(nvic_irq_t *irq_conf);
//static void copy2user(Usart *self);
// 析构函数声明
static void usart_destroy(Usart* self);

// TODO: 初始化数据成员
static const UsartFun usart_fun = {
    .destroy = usart_destroy,
};

// 构造函数实现
Usart* usart_create(const device_info_t *info) {
    if (info == NULL) {
        return NULL;
    }
    Usart* obj = (Usart*)os_malloc(sizeof(Usart));
    if (obj) {
        memset(obj, 0, sizeof(Usart));
        usart_init(obj, info);
    }
    return obj;
}

void usart_init(Usart* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(usart_fun);
    // TODO: 初始化派生类特有成员
    GET_DEVICE_VTABLE(self)->dev_init = usart_dev_init_impl;
    GET_DEVICE_VTABLE(self)->dev_read = usart_dev_read_impl;
    GET_DEVICE_VTABLE(self)->dev_write = usart_dev_write_impl;
    GET_DEVICE_VTABLE(self)->dev_ioctl = usart_dev_ioctl_impl;
    const usart_config_t *conf = info->conf;

    self->rx_cache_buf = ringbuf_create(conf->cache_size);

    //初始化uart_xfer对象,从用户空间传递发送接收buffer
    if (!self->uart_xfer) {
        self->uart_xfer = os_malloc(sizeof(uart_xfer_t));
        memset(self->uart_xfer, 0, sizeof(uart_xfer_t));
        self->uart_xfer->tx_user_buf = os_malloc(sizeof(uart_cache_t));
        memset(self->uart_xfer->tx_user_buf, 0, sizeof(uart_cache_t));
        self->uart_xfer->rx_user_buf = os_malloc(sizeof(uart_cache_t));
        memset(self->uart_xfer->rx_user_buf, 0, sizeof(uart_cache_t));
        self->uart_xfer->uart_tx_sem = semaphore_create(0);
        self->uart_xfer->uart_rx_sem = semaphore_create(0);
    }
}

void usart_deinit(Usart* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void usart_destroy(Usart* self) {
    if (self != NULL) {
        usart_deinit(self);
        os_free(self);
    }
}

// dev_init method Semaphore *sem, intc_handler_t handle, void* arg
dev_init_override(usart_dev_init_impl) {
    // TODO: add dev_init method
    Usart *usart = (Usart *)self;
    const usart_config_t *conf = self->info->conf;
    //params
    LOG_DEBUG("usart", "usart init");

    // 2. 配置引脚 (AF9)
    pin_config_t pins[2] = {
            {.mode = PIN_MODE_AF,
                    .otype = PIN_OTYPE_PP,
                    .ospeed = PIN_OSPEED_HIGH,
                    .pupd = PIN_PUPD_NONE,
                    .af = conf->pins.uart_tx },
            {.mode = PIN_MODE_AF,
                    .otype = PIN_OTYPE_PP,
                    .ospeed = PIN_OSPEED_HIGH,
                    .pupd = PIN_PUPD_NONE,
                    .af = conf->pins.uart_rx }
    };
    if (pinmux_request_group(pins, 2) != PINMUX_SUCCESS) {
        LOG_ERROR("usart", "init pinmux error!");
        return;
    }

    // 1. 使能时钟
    hal_uart_clock_enable(conf->id);
    hal_uart_set_baudrate(conf->id, conf->baudrate);
    // 4. 配置帧格式
    hal_uart_set_format(conf->id, conf->word_len, conf->stop_bits, conf->parity);
    hal_uart_enable(conf->id);
    if (conf->dma_cfg) {
        /*// 正确的初始化顺序
            dma_stream_request(&rx_dma_cfg);      // 配置 DMA 流
            USART1->CR3 |= USART_CR3_DMAR;        // 先开 DMAR
            dma_start_transfer(..., rx_buf, len); // 再启动 DMA（EN=1）*/
        if (conf->dma_cfg->tx_dma) {
            if (dma_stream_request(conf->dma_cfg->tx_dma) != DMA_SUCCESS) {
                // 申请失败，回滚
                //goto error;
                LOG_ERROR("usart", "dma request error");
                return;
            }
            if (conf->dma_cfg->tx_dma->it_enable) {
                self->irq_conf->handler = usart_txdma_irq_handler_impl;
                self->irq_conf->arg = self;
                self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg->tx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
        }
        if (conf->dma_cfg->rx_dma) {
            if (dma_stream_request(conf->dma_cfg->rx_dma) != DMA_SUCCESS) {
                //goto error;
                LOG_ERROR("usart", "dma request error");
                return;
            }
            if (conf->dma_cfg->rx_dma->it_enable) {
                self->irq_conf->handler = usart_rxdma_irq_handler_impl;
                self->irq_conf->arg = self;
                self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg->rx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
            if (usart->rx_cache_buf) {
                /* 使能 USART IDLE 中断用于帧边界检测
             * 注意：直接调用 nvic_register 注册 USART IRQ，
             * 避免通过 config_irq 重新注册已绑定的 TX DMA IRQ */
                hal_uart_it_enable(conf->id, xUART_FLAG_IDLE);
                nvic_register(gloable_nvic, USART1_IRQ + conf->id,
                              usart_dma_idle_irq_handler_impl, self, NULL);
                LOG_DEBUG("usart", "DMA RX circular buffer started, size=%d", DEFAULT_RX_BUFFER);
            } else {
                LOG_ERROR("usart", "DMA RX buffer alloc failed");
            }
        }
        hal_uart_dma_init(conf->id, conf->dma_cfg->tx_dma, conf->dma_cfg->rx_dma);

        if (conf->dma_cfg->rx_dma) {
            uart_recv_dma(conf->id, conf->dma_cfg->rx_dma,
                          usart->rx_cache_buf->buffer, usart->rx_cache_buf->size);
        }
    } else if (hal_uart_it_init(conf->id, conf->it_enable)) {
        // 6. 中断配置
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, USART1_IRQ + conf->id);
        self->irq_conf->handler = usart_irq_handler_impl;
        self->irq_conf->arg = self;
        if (!self->fun->config_irq(self, self->irq_conf)) {
            LOG_ERROR("uart", "attach irq %d error", USART1_IRQ + conf->id);
        }

    }

}
// dev_read method
dev_read_override(usart_dev_read_impl) {
    // TODO: add dev_read method
    Usart *usart = (Usart *)self;
    const usart_config_t *conf = self->info->conf;
    uart_xfer_t *rx = usart->uart_xfer;
    uint8_t *buf_ptr = (uint8_t *)buf;
    if (!rx) {
        return 0;
    }
    //params , void *buf, size_t count
    if (conf->dma_cfg && conf->dma_cfg->rx_dma) {
        /* ① 先取环形缓冲中的已有数据（上次 dev_read 返回后新来的） */
        size_t recv_size = ringbuf_get(usart->rx_cache_buf, buf_ptr, count);
        if (recv_size) {
            return recv_size;                  /* 已有数据，立即返回 */
        } else {
            rx->rx_user_buf->buf = buf_ptr;
            rx->rx_user_buf->buf_size = (uint16_t) count;
            rx->rx_user_buf->pos = 0;
            rx->rx_user_active = true;
            rx->uart_rx_sem->fun->take(rx->uart_rx_sem);
            return rx->rx_user_buf->pos;
        }
    } else if (conf->it_enable) {
        /* ① 先取环形缓冲中的已有数据（上次 dev_read 返回后新来的） */
        size_t recv_size = ringbuf_get(usart->rx_cache_buf, buf_ptr, count);
        if (recv_size) {
            return recv_size;                  /* 已有数据，立即返回 */
        } else {
            rx->rx_user_buf->buf    = buf_ptr;
            rx->rx_user_buf->buf_size  = (uint16_t)count;
            rx->rx_user_buf->pos  = 0;
            rx->rx_user_active = true;
            hal_uart_it_enable(conf->id, xUART_IT_RXNE | xUART_FLAG_IDLE);
            rx->uart_rx_sem->fun->take(rx->uart_rx_sem);
            return rx->rx_user_buf->pos;
        }
    } else {
        return hal_uart_recv(conf->id, (uint8_t *) buf, count);
    }
}
// dev_write method
dev_write_override(usart_dev_write_impl) {
    // TODO: add dev_write method
    Usart *usart = (Usart *)self;
    const usart_config_t *conf = self->info->conf;
    //params , const void *buf, size_t count
    // 检查发送数据寄存器是否为空 (TXE标志位)
    if (conf->dma_cfg) {
        uart_xfer_t *x = usart->uart_xfer;
        if (!x) {
            return ;
        }
        if (x->tx_user_active) {
            return ;   // 上次传输未结束
        }
        x->tx_user_active = true;
        uart_send_dma(conf->id, conf->dma_cfg->tx_dma, (uint8_t *) buf, count);
        usart->uart_xfer->uart_tx_sem->fun->take(usart->uart_xfer->uart_tx_sem);
    } else if (conf->it_enable) {
        uart_xfer_t *x = usart->uart_xfer;
        if (!x) {
            return ;
        }
        if (x->tx_user_active) {
            return ;   // 上次传输未结束
        }

        x->tx_user_buf->buf = (uint8_t *) buf;
        x->tx_user_buf->buf_size = count;
        x->tx_user_buf->pos = 0;
        x->tx_user_active = true;

        // 使能发送中断，并确保 TC 中断也打开（用于检测完成）
        hal_uart_it_enable(conf->id, xUART_IT_TXE | xUART_IT_TC);// TXEIE + TCIE
        x->uart_tx_sem->fun->take(x->uart_tx_sem);
    } else {
        hal_uart_send(conf->id, (uint8_t *) buf, count);
    }


}
// dev_ioctl method
dev_ioctl_override(usart_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    //Usart *usart = (Usart *)self;
    //params , int cmd, void *arg
    
}
static void usart_recv(Usart* self, uint8_t *buffer, uint16_t len) {
    const usart_config_t *conf = GET_DEVICE(self)->info->conf;
    hal_uart_recv(conf->id, (uint8_t *)buffer, len);
}
static void usart_send(Usart* self, const uint8_t *data, uint16_t len) {
    const usart_config_t *conf = GET_DEVICE(self)->info->conf;
    hal_uart_send(conf->id, (uint8_t *)data, len);
}
// irq_handler method
static bool usart_irq_handler_impl(nvic_irq_t *irq_conf) {
    // TODO: add irq_handler method
    Usart *usart = (Usart *)irq_conf->arg;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    //params , void *arg
    uart_xfer_t *x = usart->uart_xfer;
    uint32_t sr = hal_uart_get_it_event(conf->id);

    /*
     * --- 接收中断 (RXNE) ---
     *
     * ISR 永远把 DR 数据推入软件环形缓冲区（类似 DMA 推入硬件环形缓冲）。
     * RXNEIE 始终开启不关，保证 DR 每次都被读走，
     * 消除两帧之间的数据遗漏窗口。
     */
    if (sr & xUART_FLAG_RXNE) {
        uint8_t data = *hal_uart_data_addr(conf->id);  /* 无条件读 DR，防锁死 */
        if (x) {
            ringbuf_put(usart->rx_cache_buf, data);
        }
    }

    /*
     * --- IDLE 中断：帧结束检测 ---
     *
     * 清除 IDLE 标志（IT 模式无 DMA，直接 SR→DR 即可）。
     *
     * 关键：只有环形缓冲区已有数据时才通知 dev_read。
     * 系统上电后 RX 线一直空闲 → IDLE 早已置位 →
     * 若不做判断直接 signal，会使能 IDLEIE 的瞬间假唤醒。
     * 另一方面，若在 dev_read 中直接 SR→DR 清除 IDLE，
     * 可能吞掉刚到达 DR 但 ISR 还来不及推入 ring 的字符。
     * 因此判断逻辑统一放在 ISR 中，以 ring 是否非空为准。
     */
    if (sr & xUART_FLAG_IDLE) {
        volatile uint32_t dr_clear = *hal_uart_data_addr(conf->id);
        (void)dr_clear;

        /* 环形缓冲有数据才是真正的帧结束 && x->rx_active */
        if (x && usart->rx_cache_buf->count > 0) {
            hal_uart_it_disable(conf->id, xUART_FLAG_IDLE);
            x->rx_user_active = false;
            if (x->rx_user_buf->buf) {
                size_t recv_size = ringbuf_get(usart->rx_cache_buf, x->rx_user_buf->buf, x->rx_user_buf->buf_size);
                x->rx_user_buf->pos = recv_size;
                x->rx_user_buf->buf = NULL;
                x->uart_rx_sem->fun->give(x->uart_rx_sem);
            }
        }
        /* ring 为空 → 假 IDLE（线路空闲但无数据），仅清标志，继续等 */
    }

    // --- 发送中断 (TXE) ---
    if (sr & xUART_FLAG_TXE) {
        if (x && x->tx_user_active && x->tx_user_buf->pos < x->tx_user_buf->buf_size) {
            *hal_uart_data_addr(conf->id) = x->tx_user_buf->buf[x->tx_user_buf->pos++];
        } else {
            hal_uart_it_disable(conf->id, xUART_IT_TXE);
            if (x && x->tx_user_active) {
                x->tx_user_active = false;
                x->uart_tx_sem->fun->give(x->uart_tx_sem);
            }
        }
    }

    // --- 发送完成中断 (TC) ---
    if (sr & xUART_FLAG_TC) {
        hal_uart_it_clear(conf->id, xUART_IT_TC);
        if (x && x->tx_user_active && x->tx_user_buf->pos >= x->tx_user_buf->buf_size) {
            x->tx_user_active = false;
            x->uart_tx_sem->fun->give(x->uart_tx_sem);
        }
    }

    /*
     * --- 溢出错误处理 (ORE) ---
     * ORE 清除序列：读 SR（ISR 入口已做），再读 DR。
     * 必须独立处理——ORE 在 USART 中单独触发，不与 RXNE 重叠。
     */
    if (sr & xUART_FLAG_ORE) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id);
        (void)dr;
    }

    // --- 其他错误处理 ---
    if (sr & (xUART_FLAG_PE | xUART_FLAG_FE | xUART_FLAG_NE)) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id);
        (void)dr;
    }
    return true;
}

static bool usart_txdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Usart *usart = (Usart *)irq_conf->arg;
    uart_xfer_t *tx = usart->uart_xfer;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    tx->tx_user_active = false;
    usart->uart_xfer->uart_tx_sem->fun->give(usart->uart_xfer->uart_tx_sem);
    return true;
}

static bool usart_rxdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Usart *usart = (Usart *)irq_conf->arg;
    uart_xfer_t *rx = usart->uart_xfer;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    dma_it_event_t it_event = dma_get_it_event(conf->dma_cfg->rx_dma);
    dma_clear_flag(conf->dma_cfg->rx_dma);
    volatile uint16_t cur_ndtr = uart_dma_get_rx_ndtr(conf->dma_cfg->rx_dma);
    size_t dma_pos = usart->rx_cache_buf->size - cur_ndtr;
    if (it_event == DMA_IT_EVENT_HTIF) {//半传输中断
        if (rx->rx_user_buf->buf) {
           ringbuf_dma_update(usart->rx_cache_buf, dma_pos);
           size_t recv_size = ringbuf_get(usart->rx_cache_buf, rx->rx_user_buf->buf + rx->rx_user_buf->pos, rx->rx_user_buf->buf_size - rx->rx_user_buf->pos);
           rx->rx_user_buf->pos += recv_size;
        }
    }
    if (it_event == DMA_IT_EVENT_TCIF) {//传输中断
        if (rx->rx_user_buf->buf) {
            ringbuf_dma_update(usart->rx_cache_buf, dma_pos);
            size_t recv_size = ringbuf_get(usart->rx_cache_buf, rx->rx_user_buf->buf + rx->rx_user_buf->pos, rx->rx_user_buf->buf_size - rx->rx_user_buf->pos);
            rx->rx_user_buf->pos += recv_size;
        }
    }

    return true;
}
/*
 * USART IDLE 中断处理（DMA 接收模式）
 * IDLE 帧表示一帧数据接收完毕，通过 NDTR 计算实际接收字节数，
 * 从 DMA 环形缓冲区拷贝到用户缓冲区，然后释放信号量。
 */
static bool usart_dma_idle_irq_handler_impl(nvic_irq_t *irq_conf) {
    Usart *usart = (Usart *)irq_conf->arg;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    uint32_t sr = hal_uart_get_it_event(conf->id);
    /* --- IDLE 中断：检测到帧空闲 --- */
    if (sr & xUART_FLAG_IDLE) {
        if (!conf->dma_cfg || !conf->dma_cfg->rx_dma) {
            return true;
        }

        uart_xfer_t *rx = usart->uart_xfer;
        hal_uart_clear_idle_flag(conf->id, conf->dma_cfg->rx_dma);
        volatile uint16_t cur_ndtr = uart_dma_get_rx_ndtr(conf->dma_cfg->rx_dma);

        size_t dma_pos = usart->rx_cache_buf->size - cur_ndtr;
        ringbuf_dma_update(usart->rx_cache_buf, dma_pos);
        rx->rx_user_active = false;
        /* 如果 dev_read 在等待，拷贝数据到用户缓冲区 */
        if (rx->rx_user_buf->buf) {
            size_t recv_size = ringbuf_get(usart->rx_cache_buf, rx->rx_user_buf->buf + rx->rx_user_buf->pos, rx->rx_user_buf->buf_size - rx->rx_user_buf->pos);
            rx->rx_user_buf->pos += recv_size;
            rx->rx_user_buf->buf = NULL;
            rx->uart_rx_sem->fun->give(rx->uart_rx_sem);
        }
    }

    /* --- 溢出错误处理 --- */
    if (sr & xUART_FLAG_ORE) {
        volatile uint32_t dr = *hal_uart_data_addr(conf->id);
        (void)dr;
    }

    return true;
}

// send_it method
static void usart_send_it(Usart* self, const uint8_t *data, uint16_t len) {
    const usart_config_t *conf = GET_DEVICE(self)->info->conf;
    if (conf->id >= UART_MAX || len == 0) {
        return;
    }
    uart_xfer_t *x = self->uart_xfer;
    if (!x) {
        return;
    }
    if (x->tx_user_active) {
        return;   // 上次传输未结束
    }

    x->tx_user_buf->buf = data;
    x->tx_user_buf->buf_size = len;
    x->tx_user_buf->pos = 0;
    x->tx_user_active = true;

    // 使能发送中断，并确保 TC 中断也打开（用于检测完成）
    hal_uart_it_enable(conf->id, xUART_IT_TXE | xUART_IT_TC);// TXEIE + TCIE
    //self->uart_tx_sem->fun->take(self->uart_tx_sem);
}


// recv_it method
static void usart_recv_it(Usart* self, uint8_t *buffer, uint16_t len) {
    const usart_config_t *conf = GET_DEVICE(self)->info->conf;
    if (conf->id >= UART_MAX || len == 0) {
        return;
    }
    uart_xfer_t *x = self->uart_xfer;
    if (!x) {
        return;
    }
    if (x->rx_user_active) {
        return;
    }

    x->rx_user_buf->buf = buffer;
    x->rx_user_buf->buf_size = len;
    x->rx_user_buf->pos = 0;
    x->rx_user_active = true;
    hal_uart_it_enable(conf->id, xUART_IT_RXNE);// RXNEIE
    x->uart_rx_sem->fun->take(x->uart_rx_sem);
}


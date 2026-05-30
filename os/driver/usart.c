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
// 析构函数声明
static void usart_destroy(Usart* self);

// TODO: 初始化数据成员
static const UsartFun usart_fun = {
    .destroy = usart_destroy,
	.send_it = usart_send_it,
	.recv_it = usart_recv_it,
    .send = usart_send,
    .recv = usart_recv,
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
        LOG_DEBUG("can", "init pinmux error!");
        return;
    }

    // 1. 使能时钟
    hal_uart_clock_enable(conf->id);
    hal_uart_set_baudrate(conf->id, conf->baudrate);
    // 4. 配置帧格式
    hal_uart_set_format(conf->id, conf->word_len, conf->stop_bits, conf->parity);

    hal_uart_enable(conf->id);

    // 6. 中断配置
    if (hal_uart_it_init(conf->id, conf->it_enable)) {
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, USART1_IRQ + conf->id);
        self->irq_conf->handler = usart_irq_handler_impl;
        self->irq_conf->arg = self;
        if (!self->fun->config_irq(self, self->irq_conf)) {
            LOG_ERROR("systick", "attach irq %d error", USART1_IRQ + conf->id);
        }
        if (!usart->uart_xfer) {
            usart->uart_xfer = os_malloc(sizeof(uart_xfer_t));
            memset(usart->uart_xfer, 0, sizeof(uart_xfer_t));
            usart->uart_xfer->uart_tx_sem = semaphore_create(0);
            usart->uart_xfer->uart_rx_sem = semaphore_create(0);
        }
    }
    if (conf->dma_cfg) {
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
                return;
            }
            if (conf->dma_cfg->rx_dma->it_enable) {
                self->irq_conf->handler = usart_rxdma_irq_handler_impl;
                self->irq_conf->arg = self;

                self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg->rx_dma));
                self->fun->config_irq(self, self->irq_conf);
            }
        }
        hal_uart_dma_init(conf->id, conf->dma_cfg->tx_dma, conf->dma_cfg->rx_dma);
    }

}
// dev_read method
dev_read_override(usart_dev_read_impl) {
    // TODO: add dev_read method
    Usart *usart = (Usart *)self;
    const usart_config_t *conf = self->info->conf;
    //params , void *buf, size_t count
    if (conf->dma_cfg) {

    } else if (conf->it_enable) {
        uart_xfer_t *x = usart->uart_xfer;
        if (!x) {
            return;
        }
        if (x->rx_active) {
            return;
        }

        x->rx_buf = (uint8_t *) buf;
        x->rx_total = count;
        x->rx_index = 0;
        x->rx_active = true;
        hal_uart_set_it_event(conf->id, xUART_IT_RXNE);// RXNEIE
        x->uart_rx_sem->fun->take(x->uart_rx_sem);
    } else {
        hal_uart_recv(conf->id, (uint8_t *) buf, count);
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

    } else if (conf->it_enable) {
        uart_xfer_t *x = usart->uart_xfer;
        if (!x) {
            return;
        }
        if (x->tx_active) {
            return;   // 上次传输未结束
        }

        x->tx_buf = (uint8_t *) buf;
        x->tx_total = count;
        x->tx_index = 0;
        x->tx_active = true;

        // 使能发送中断，并确保 TC 中断也打开（用于检测完成）
        hal_uart_set_it_event(conf->id, xUART_IT_TXE | xUART_IT_TC);// TXEIE + TCIE
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
    // 检查SR寄存器的RXNE位，表示接收到了新数据
    uart_xfer_t *x = usart->uart_xfer;
    //if (!x) {
    //    return true;
    //}
    uint32_t sr = hal_uart_get_it_event(conf->id);
   // uint8_t data;

    // --- 接收中断 (RXNE) ---
    if (sr & xUART_FLAG_RXNE) {  // RXNE
        if (x && x->rx_active && x->rx_index < x->rx_total) {
            x->rx_buf[x->rx_index++] = *hal_uart_data_addr(conf->id);  // 读取数据自动清 RXNE
        }
        if (x && x->rx_active && x->rx_index >= x->rx_total) {
            x->rx_active = false;
            hal_uart_clear_it_event(conf->id, xUART_IT_RXNE);// 关闭 RXNE 中断
            x->uart_rx_sem->fun->give(x->uart_rx_sem);
        }
    }

    // --- 发送中断 (TXE) ---
    if (sr & xUART_FLAG_TXE) {  // TXE
        if (x && x->tx_active && x->tx_index < x->tx_total) {
            *hal_uart_data_addr(conf->id) = x->tx_buf[x->tx_index++];
        } else {
            // 缓冲区已空，关闭 TXE 中断，保留 TC 中断用于完成通知
            hal_uart_clear_it_event(conf->id, xUART_IT_TXE);
            if (x && x->tx_active) {
                x->tx_active = false;
                x->uart_tx_sem->fun->give(x->uart_tx_sem);
            }
        }
    }

    // --- 发送完成中断 (TC) ---
    if (sr & xUART_FLAG_TC) {  // TC
        // 清除 TC 标志：先读 SR，再写 DR 无效（TC 是软件清除，写0到SR的TC位）
        // 但 USART 的 TC 标志是通过写 0 清？实际上写 0 无效，需要读 SR + 写 DR？不，TC 清法：直接向 SR 的 TC 位写 0 即可（F4手册：通过对 USART_SR 寄存器的 TC 位写 0 来清除）
        hal_uart_clear_it_event(conf->id, xUART_IT_TC);
        if (x && x->tx_active && x->tx_index >= x->tx_total) {
            x->tx_active = false;
            x->uart_tx_sem->fun->give(x->uart_tx_sem);
        }
    }

    // --- 错误处理 ---
    if (sr & xUART_FLAG_PE || sr & xUART_FLAG_FE || sr & xUART_FLAG_NE || sr & xUART_FLAG_ORE) {
        // PE, FE, NF, ORE 等
        uint32_t dr = *hal_uart_data_addr(conf->id);  // 读 DR 可清除部分错误标志
        (void)dr;
    }
    return true;
}

static bool usart_txdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Usart *usart = (Usart *)irq_conf->arg;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    dma_clear_flag(conf->dma_cfg->tx_dma);
    return true;
}

static bool usart_rxdma_irq_handler_impl(nvic_irq_t *irq_conf) {
    Usart *usart = (Usart *)irq_conf->arg;
    const usart_config_t *conf = GET_DEVICE(usart)->info->conf;
    dma_clear_flag(conf->dma_cfg->rx_dma);
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
    if (x->tx_active) {
        return;   // 上次传输未结束
    }

    x->tx_buf = data;
    x->tx_total = len;
    x->tx_index = 0;
    x->tx_active = true;

    // 使能发送中断，并确保 TC 中断也打开（用于检测完成）
    hal_uart_set_it_event(conf->id, xUART_IT_TXE | xUART_IT_TC);// TXEIE + TCIE
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
    if (x->rx_active) {
        return;
    }

    x->rx_buf = buffer;
    x->rx_total = len;
    x->rx_index = 0;
    x->rx_active = true;
    hal_uart_set_it_event(conf->id, xUART_IT_RXNE);// RXNEIE
    x->uart_rx_sem->fun->take(x->uart_rx_sem);
}


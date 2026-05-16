#include "usart.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include "../log/log.h"
#include "common/rcc.h"
#include "hal/hal_usart.h"

static void usart_recv_it(Usart* self, uint8_t *buffer, uint16_t len);

static void usart_send_it(Usart* self, const uint8_t *data, uint16_t len);

dev_init_override(usart_dev_init_impl);
dev_read_override(usart_dev_read_impl);
dev_write_override(usart_dev_write_impl);
dev_ioctl_override(usart_dev_ioctl_impl);
static bool usart_irq_handler_impl(void *arg);
static bool usart_txdma_irq_handler_impl(void *arg);
static bool usart_rxdma_irq_handler_impl(void *arg);
// 析构函数声明
static void usart_destroy(Usart* self);

// TODO: 初始化数据成员
static const UsartFun usart_fun = {
    .destroy = usart_destroy,
	.send_it = usart_send_it,
	.recv_it = usart_recv_it,
};

// 构造函数实现
Usart* usart_create(const usart_config * conf) {
    if (conf == NULL) {
        return NULL;
    }
    Usart* obj = (Usart*)os_malloc(sizeof(Usart));
    if (obj) {
        memset(obj, 0, sizeof(Usart));
        usart_init(obj, conf);
    }
    return obj;
}

void usart_init(Usart* self, const usart_config * conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(usart_fun);
    // TODO: 初始化派生类特有成员

    GET_DEVICE_VTABLE(self)->dev_init = usart_dev_init_impl;
    GET_DEVICE_VTABLE(self)->dev_read = usart_dev_read_impl;
    GET_DEVICE_VTABLE(self)->dev_write = usart_dev_write_impl;
    GET_DEVICE_VTABLE(self)->dev_ioctl = usart_dev_ioctl_impl;
    //self->rx_complete = 0;
   // self->rx_index = 0;
    self->conf = conf;
    self->uart_tx_sem = semaphore_create(0);
    self->uart_rx_sem = semaphore_create(0);
    //self->rx_size = conf->buffer_size == 0 ? DEFAULT_RX_BUFFER : conf->buffer_size;
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
    //params
    LOG_DEBUG("usart", "usart init");
    if (pinmux_request(usart->conf->tx_conf) == PINMUX_ERROR) {
        LOG_ERROR("usart", "tx pinmux error");
    }

    if (pinmux_request(usart->conf->rx_conf) == PINMUX_ERROR) {
        LOG_ERROR("usart", "rx pinmux error");
    }

    // 使能UART4时钟 (APB1总线，位19)
    // 1. 使能时钟
    hal_uart_clock_enable(usart->conf->id);
    xUSART_TypeDef *uart_ctrl = USARTx[usart->conf->id];
    // 3. 波特率 (假设 APB2=84MHz for USART1, APB1=42MHz for USART2/3)
    uint32_t pclk = (usart->conf->id == UART_1) ? 84000000UL : 42000000UL;
    uart_ctrl->BRR = pclk / usart->conf->baudrate;
    // 4. 配置帧格式
    uint32_t cr1 = (1 << 3) | (1 << 2);  // TE, RE
    cr1 |= (usart->conf->word_len & 0x01) << 12; // M
    cr1 |= (usart->conf->parity & 0x03) << 9;   // PS, PCE
    uart_ctrl->CR1 = cr1;

    uart_ctrl->CR2 = (usart->conf->stop_bits & 0x03) << 12;

    // 5. 使能模块
    uart_ctrl->CR1 |= (1 << 13);  // UE

    // 6. 中断配置
    if (usart->conf->it_enable) {
        xUSART_TypeDef *uart_ctrl = USARTx[usart->conf->id];
        uint32_t cr1 = uart_ctrl->CR1;
        if (usart->conf->it_enable & xUART_IT_TXE) {
            cr1 |= (1 << 7);   // TXEIE
        }
        if (usart->conf->it_enable & xUART_IT_RXNE) {
            cr1 |= (1 << 5);   // RXNEIE
        }
        if (usart->conf->it_enable & xUART_IT_TC) {
            cr1 |= (1 << 6);   // TCIE
        }
        uart_ctrl->CR1 = cr1;
        self->irq_conf.irq_num = USART1_IRQ + usart->conf->id;
        self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);
        self->irq_conf.handler = usart_irq_handler_impl;
        self->irq_conf.semaphore = sem;
        self->irq_conf.arg = self;

        if (!self->fun->attach_irq(self, &self->irq_conf)) {
            LOG_ERROR("systick", "attach irq %d error", self->irq_conf.irq_num);
        }
    }

    if (usart->conf->dma_cfg && usart->conf->dma_cfg->tx_dma) {
        if (dma_stream_request(usart->conf->dma_cfg->tx_dma) != DMA_SUCCESS) {
            // 申请失败，回滚
            //goto error;
            return;
        }
        // 使能 USART 的 DMA 发送位 CR3 bit7 (DMAT)
        USARTx[usart->conf->id]->CR3 |= (1 << 7);
        if (usart->conf->dma_cfg->tx_dma->it_enable) {
            self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
            self->irq_conf.handler = usart_txdma_irq_handler_impl;
            self->irq_conf.semaphore = sem;
            self->irq_conf.arg = self;
            self->irq_conf.irq_num = dma_get_irqnum(usart->conf->dma_cfg->tx_dma);
            self->fun->attach_irq(self, &self->irq_conf);
        }
    }
    if (usart->conf->dma_cfg && usart->conf->dma_cfg->rx_dma) {
        if (dma_stream_request(usart->conf->dma_cfg->rx_dma) != DMA_SUCCESS) {
            //goto error;
            return;
        }
        USARTx[usart->conf->id]->CR3 |= (1 << 6); // DMAR
        if (usart->conf->dma_cfg->rx_dma->it_enable) {
            self->irq_conf.priority = self->fun->encode_pripority(self, 0x02, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
            self->irq_conf.handler = usart_rxdma_irq_handler_impl;
            self->irq_conf.semaphore = sem;
            self->irq_conf.arg = self;

            self->irq_conf.irq_num = dma_get_irqnum(usart->conf->dma_cfg->rx_dma);
            self->fun->attach_irq(self, &self->irq_conf);
        }
    }

}
// dev_read method
dev_read_override(usart_dev_read_impl) {
    // TODO: add dev_read method
    Usart *usart = (Usart *)self;
    //params , void *buf, size_t count
    while(!(USARTx[usart->conf->id]->SR & (1 << 5)));
    *(char *)buf = (char)USARTx[usart->conf->id]->DR;
}
// dev_write method
dev_write_override(usart_dev_write_impl) {
    // TODO: add dev_write method
    Usart *usart = (Usart *)self;
    //params , const void *buf, size_t count
    // 检查发送数据寄存器是否为空 (TXE标志位)
    while (count--) {
        while (!(USARTx[usart->conf->id]->SR & (1 << 7)));
        USARTx[usart->conf->id]->DR = *(char *) buf++;
    }

}
// dev_ioctl method
dev_ioctl_override(usart_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    //Usart *usart = (Usart *)self;
    //params , int cmd, void *arg
    
}

// irq_handler method
static bool usart_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    Usart *usart = (Usart *)arg;
    //params , void *arg
    // 检查SR寄存器的RXNE位，表示接收到了新数据
    xUSART_TypeDef *uart_ctrl = USARTx[usart->conf->id];
    uart_xfer_t *x = &usart->uart_xfer;
    uint32_t sr = uart_ctrl->SR;
    uint8_t data;

    // --- 接收中断 (RXNE) ---
    if (sr & (1 << 5)) {  // RXNE
        data = (uint8_t)uart_ctrl->DR;   // 读取数据自动清 RXNE
        if (x->rx_active && x->rx_index < x->rx_total) {
            x->rx_buf[x->rx_index++] = data;
        }
        if (x->rx_active && x->rx_index >= x->rx_total) {
            x->rx_active = false;
            uart_ctrl->CR1 &= ~(1 << 5);   // 关闭 RXNE 中断
            usart->uart_rx_sem->fun->give(usart->uart_rx_sem);
        }
    }

    // --- 发送中断 (TXE) ---
    if (sr & (1 << 7)) {  // TXE
        if (x->tx_active && x->tx_index < x->tx_total) {
            uart_ctrl->DR = x->tx_buf[x->tx_index++];
        } else {
            // 缓冲区已空，关闭 TXE 中断，保留 TC 中断用于完成通知
            uart_ctrl->CR1 &= ~(1 << 7);   // 关 TXEIE
        }
    }

    // --- 发送完成中断 (TC) ---
    if (sr & (1 << 6)) {  // TC
        // 清除 TC 标志：先读 SR，再写 DR 无效（TC 是软件清除，写0到SR的TC位）
        // 但 USART 的 TC 标志是通过写 0 清？实际上写 0 无效，需要读 SR + 写 DR？不，TC 清法：直接向 SR 的 TC 位写 0 即可（F4手册：通过对 USART_SR 寄存器的 TC 位写 0 来清除）
        uart_ctrl->SR &= ~(1 << 6);  // 清除 TC
        if (x->tx_active && x->tx_index >= x->tx_total) {
            x->tx_active = false;
            uart_ctrl->CR1 &= ~(1 << 6);   // 关闭 TCIE
            usart->uart_tx_sem->fun->give(usart->uart_tx_sem);
        }
    }

    // --- 错误处理 ---
    if (sr & (1 << 0) || sr & (1 << 1) || sr & (1 << 2) || sr & (1 << 3)) {
        // PE, FE, NF, ORE 等
        uint32_t dr = uart_ctrl->DR;  // 读 DR 可清除部分错误标志
        (void)dr;
    }
    return true;
}

static bool usart_txdma_irq_handler_impl(void *arg) {
    Usart *usart = (Usart *)arg;
    dma_clear_flag(usart->conf->dma_cfg->tx_dma);
    return true;
}

static bool usart_rxdma_irq_handler_impl(void *arg) {
    Usart *usart = (Usart *)arg;
    dma_clear_flag(usart->conf->dma_cfg->rx_dma);
    return true;
}




// send_it method
static void usart_send_it(Usart* self, const uint8_t *data, uint16_t len) {
    if (self->conf->id >= UART_MAX || len == 0) {
        return;
    }
    uart_xfer_t *x = &self->uart_xfer;
    if (x->tx_active) {
        return;   // 上次传输未结束
    }

    x->tx_buf = data;
    x->tx_total = len;
    x->tx_index = 0;
    x->tx_active = true;

    xUSART_TypeDef *uart = USARTx[self->conf->id];
    // 使能发送中断，并确保 TC 中断也打开（用于检测完成）
    uart->CR1 |= (1 << 7) | (1 << 6);   // TXEIE + TCIE
    self->uart_tx_sem->fun->take(self->uart_tx_sem);
}


// recv_it method
static void usart_recv_it(Usart* self, uint8_t *buffer, uint16_t len) {
    if (self->conf->id >= UART_MAX || len == 0) {
        return;
    }
    uart_xfer_t *x = &self->uart_xfer;
    if (x->rx_active) {
        return;
    }

    x->rx_buf = buffer;
    x->rx_total = len;
    x->rx_index = 0;
    x->rx_active = true;

    xUSART_TypeDef *uart = USARTx[self->conf->id];
    uart->CR1 |= (1 << 5);   // RXNEIE
    self->uart_rx_sem->fun->take(self->uart_rx_sem);
}


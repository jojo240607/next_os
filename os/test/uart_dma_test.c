/**
 * UART DMA 发送/接收测试任务
 *
 * 功能：
 *   1. 每秒通过 USART1 DMA 发送测试数据
 *   2. 使用 USART1 DMA 循环接收，收到数据后回显
 *
 * 硬件连接：
 *   - USART1 TX: PA9  (AF7)
 *   - USART1 RX: PA10 (AF7)
 *   - 波特率: 115200, 8N1
 *
 * DMA 配置（已在 device_config.c 中定义）：
 *   - TX: DMA2 Stream7 Channel4 (DMA2_REQ_USART1_TX)
 *         M2P, 字节宽度, 存储器自增, 单次模式, TC 中断
 *   - RX: DMA2 Stream2 Channel4 (DMA2_REQ_USART1_RX_ST2)
 *         P2M, 字节宽度, 存储器自增, 循环模式, IDLE 中断
 */

#include "uart_dma_test.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../driver/svc.h"
#include "../driver/device_manager.h"

/* 虚函数实现 */
task_init_override(uart_dma_test_init_impl);
task_thread_override(uart_dma_test_thread_impl);
task_start_override(uart_dma_test_start_impl);

/* 析构函数声明 */
static void uart_dma_test_destroy(UartDmaTest* self);

static const UartDmaTestFun uart_dma_test_fun = {
    .destroy = uart_dma_test_destroy,
};

/* 构造函数 */
UartDmaTest* uart_dma_test_create(const task_into_t *info)
{
    UartDmaTest* obj = (UartDmaTest*)os_malloc(sizeof(UartDmaTest));
    if (obj) {
        memset(obj, 0, sizeof(UartDmaTest));
        uart_dma_test_init(obj, info);
    }
    return obj;
}

void uart_dma_test_init(UartDmaTest* self, const task_into_t *info)
{
    task_init(&self->base, info);
    self->fun = &(uart_dma_test_fun);

    /* 挂载虚函数 */
    def_task_init(self)   = uart_dma_test_init_impl;
    def_task_thread(self) = uart_dma_test_thread_impl;
    def_task_start(self)  = uart_dma_test_start_impl;

    /* 通过设备管理器打开 USART1（会自动完成 DMA TX/RX 初始化） */
    self->usart4 = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_USART1);
    self->send_count = 0;
    self->recv_count = 0;

    if (self->usart4) {
        LOG_DEBUG("uart_dma", "USART1 device opened successfully");
    } else {
        LOG_ERROR("uart_dma", "Failed to open USART1 device");
    }
}

void uart_dma_test_deinit(UartDmaTest* self)
{
    task_deinit(GET_TASK(self));
}

static void uart_dma_test_destroy(UartDmaTest* self)
{
    if (self != NULL) {
        uart_dma_test_deinit(self);
        os_free(self);
    }
}

/* task_init 虚函数实现 */
task_init_override(uart_dma_test_init_impl)
{
    UartDmaTest *test = (UartDmaTest *)self;
    LOG_DEBUG("uart_dma", "uart_dma_test init, task=%s",
              self->task_tcb ? self->task_tcb->name : "unknown");
    (void)test;
}

/* task_start 虚函数实现 */
task_start_override(uart_dma_test_start_impl)
{
    UartDmaTest *test = (UartDmaTest *)self;
    LOG_DEBUG("uart_dma", "uart_dma_test started");
    (void)test;
}

/* task_thread 虚函数实现 —— 发送 + 接收回显 */
task_thread_override(uart_dma_test_thread_impl)
{
    UartDmaTest *test = (UartDmaTest *)self->parent;

    /* 等待设备初始化完成 */
    sleep_user(100);

    if (!test->usart4) {
        LOG_ERROR("uart_dma", "USART4 device not available, task exit");
        while (1) { sleep_user(1000); }
    }

    LOG_DEBUG("uart_dma", "===== UART DMA send+recv test started =====");

    while (true) {
        test->send_count++;

        /* ──── 1. DMA 发送测试数据 ──── */

        int len = snprintf(test->tx_buffer, sizeof(test->tx_buffer),
                           "[%lu] DMA+USART1 Hello! Send a line and I'll echo it back.\r\n",
                           (unsigned long)test->send_count);

        LOG_DEBUG("uart_dma", "DMA send: %d bytes (count=%lu)",
                  len, (unsigned long)test->send_count);

        //test->usart1->vtable->dev_write(test->usart1, tx_buffer, len);
        test->usart4->fun->write_user(test->usart4, test->tx_buffer, len);
        LOG_DEBUG("uart_dma", "DMA send complete!");

        /* ──── 2. DMA 接收回显（等待上位机发送一行数据） ──── */

        memset(test->rx_buffer, 0, sizeof(test->rx_buffer));

        LOG_DEBUG("uart_dma", "------------------------------Waiting for DMA receive...");
        //test->usart1->vtable->dev_read(test->usart1, rx_buffer, sizeof(rx_buffer) - 1);
        test->usart4->fun->read_user(test->usart4, test->rx_buffer, sizeof(test->rx_buffer) - 1);
        LOG_DEBUG("uart_dma","\n\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxread_user %s\n\n", test->rx_buffer);
        /* 确保字符串终止 */
        test->rx_buffer[sizeof(test->rx_buffer) - 1] = '\0';
        int rx_len = 0;
        for (int i = 0; i < (int)sizeof(test->rx_buffer); i++) {
            if (test->rx_buffer[i] == '\0') break;
            if (test->rx_buffer[i] == '\r' || test->rx_buffer[i] == '\n') {
                test->rx_buffer[i] = '\0';
                rx_len = i;
                break;
            }
            rx_len = i + 1;
        }

        test->recv_count++;
        LOG_DEBUG("uart_dma", "DMA recv: %d bytes: \"%s\" (recv_count=%lu)",
                  rx_len, test->rx_buffer, (unsigned long)test->recv_count);

        /* ──── 3. DMA 发送回显 ──── */
        if (rx_len > 0) {

            int echo_len = snprintf(test->echo_buf, sizeof(test->echo_buf),
                                    "\n\n\n----------------------[Echo #%lu] You sent: %s\r\n",
                                    (unsigned long)test->recv_count, test->rx_buffer);
            //test->usart1->vtable->dev_write(test->usart1, echo_buf, echo_len);
            test->usart4->fun->write_user(test->usart4, test->echo_buf, echo_len);
            LOG_DEBUG("uart_dma", "Echo reply sent: %d bytes", echo_len);
        }

        /* 每秒一轮 */
        sleep_user(1000);
    }
}

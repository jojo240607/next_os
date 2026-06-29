/**
 * USB CDC 发送/接收回环测试任务
 *
 * 功能：
 *   1. 每隔 2s 通过 USB CDC 发送测试数据
 *   2. 接收上位机数据后回显
 *
 * 硬件连接：
 *   - USB DM: PA11 (AF10)
 *   - USB DP: PA12 (AF10)
 */
#include "usb_cdc_test.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../driver/svc.h"
#include "../driver/device_manager.h"

/* 虚函数实现 */
task_init_override(usb_cdc_test_init_impl);
task_thread_override(usb_cdc_test_thread_impl);
task_start_override(usb_cdc_test_start_impl);

/* 析构函数声明 */
static void usb_cdc_test_destroy(UsbCdcTest* self);

static const UsbCdcTestFun usb_cdc_test_fun = {
    .destroy = usb_cdc_test_destroy,
};

/* 构造函数 */
UsbCdcTest* usb_cdc_test_create(const task_into_t *info)
{
    UsbCdcTest* obj = (UsbCdcTest*)os_malloc(sizeof(UsbCdcTest));
    if (obj) {
        memset(obj, 0, sizeof(UsbCdcTest));
        usb_cdc_test_init(obj, info);
    }
    return obj;
}

void usb_cdc_test_init(UsbCdcTest* self, const task_into_t *info)
{
    task_init(&self->base, info);
    self->fun = &(usb_cdc_test_fun);

    /* 挂载虚函数 */
    def_task_init(self)   = usb_cdc_test_init_impl;
    def_task_thread(self) = usb_cdc_test_thread_impl;
    def_task_start(self)  = usb_cdc_test_start_impl;

    /* 通过设备管理器打开 USB CDC */
    self->usb = (Usb_cdc *)gloable_deviceManager->fun->dev_open(
                     gloable_deviceManager, DEVICE_USB_CDC);
    self->send_count = 0;
    self->recv_count = 0;

    if (self->usb) {
        LOG_DEBUG("usb_cdc", "USB CDC device opened successfully");
    } else {
        LOG_ERROR("usb_cdc", "Failed to open USB CDC device");
    }
}

void usb_cdc_test_deinit(UsbCdcTest* self)
{
    task_deinit(GET_TASK(self));
}

static void usb_cdc_test_destroy(UsbCdcTest* self)
{
    if (self != NULL) {
        usb_cdc_test_deinit(self);
        os_free(self);
    }
}

/* task_init 虚函数实现 */
task_init_override(usb_cdc_test_init_impl)
{
    UsbCdcTest *test = (UsbCdcTest *)self;
    LOG_DEBUG("usb_cdc", "usb_cdc_test init, task=%s",
              self->task_tcb ? self->task_tcb->name : "unknown");
    (void)test;
}

/* task_start 虚函数实现 */
task_start_override(usb_cdc_test_start_impl)
{
    UsbCdcTest *test = (UsbCdcTest *)self;
    LOG_DEBUG("usb_cdc", "usb_cdc_test started, waiting for USB connect...");
    (void)test;
}

/* task_thread 虚函数实现 —— 发送 + 接收回显 */
task_thread_override(usb_cdc_test_thread_impl)
{
    UsbCdcTest *test = (UsbCdcTest *)self->parent;

    /* 等待 USB 总线就绪 + 主机枚举完成 */
    sleep_user(2000);

    /* 调试：打印 USB 寄存器状态 */
    //hal_usb_debug_dump_regs();

    if (!test->usb) {
        LOG_ERROR("usb_cdc", "USB CDC device not available, task exit");
        while (1) { sleep_user(1000); }
    }

    LOG_DEBUG("usb_cdc", "===== USB CDC send+recv test started =====");

    while (true) {
        extern volatile uint32_t usb_irq_total, usb_setup_total, usb_oep_raw;
        LOG_ERROR("usb_cdc", "irq=%lu setup=%lu oep=%08lX", usb_irq_total, usb_setup_total, usb_oep_raw);
        test->send_count++;

        /* ──── 1. USB CDC 发送测试数据 ──── */

        int len = snprintf(test->tx_buffer, sizeof(test->tx_buffer),
                           "[%lu] Hello from STM32 USB CDC! Send a line and I'll echo it back.\r\n",
                           (unsigned long)test->send_count);

        LOG_DEBUG("usb_cdc", "USB CDC send: %d bytes (count=%lu)",
                  len, (unsigned long)test->send_count);

        GET_DEVICE(test->usb)->fun->write_user(GET_DEVICE(test->usb), test->tx_buffer, len);
        LOG_DEBUG("usb_cdc", "USB CDC send complete!");

        /* ──── 2. USB CDC 接收回显（等待上位机发送一行数据） ──── */

        memset(test->rx_buffer, 0, sizeof(test->rx_buffer));

        LOG_DEBUG("usb_cdc", "------------------------------Waiting for USB CDC receive...");
        GET_DEVICE(test->usb)->fun->read_user(GET_DEVICE(test->usb), test->rx_buffer, sizeof(test->rx_buffer) - 1);
        test->rx_buffer[sizeof(test->rx_buffer) - 1] = '\0';

        /* 去除 \r\n */
        int data_len = 0;
        for (int i = 0; i < (int)sizeof(test->rx_buffer); i++) {
            if (test->rx_buffer[i] == '\0') break;
            if (test->rx_buffer[i] == '\r' || test->rx_buffer[i] == '\n') {
                test->rx_buffer[i] = '\0';
                data_len = i;
                break;
            }
            data_len = i + 1;
        }

        test->recv_count++;
        LOG_DEBUG("usb_cdc", "USB CDC recv: %d bytes: \"%s\" (recv_count=%lu)",
                  data_len, test->rx_buffer, (unsigned long)test->recv_count);

        /* ──── 3. USB CDC 发送回显 ──── */
        if (data_len > 0) {
            int echo_len = snprintf(test->echo_buf, sizeof(test->echo_buf),
                                    "\r\n----------------------[Echo #%lu] You sent: %s\r\n",
                                    (unsigned long)test->recv_count, test->rx_buffer);
            GET_DEVICE(test->usb)->fun->write_user(GET_DEVICE(test->usb), test->echo_buf, echo_len);
            LOG_DEBUG("usb_cdc", "Echo reply sent: %d bytes", echo_len);
        }

        /* 每轮 2 秒 */
        sleep_user(1000);
    }
}

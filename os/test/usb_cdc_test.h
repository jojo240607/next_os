/**
 * USB CDC 回环测试任务
 *
 * 功能：
 *   1. 每隔 2s 通过 USB CDC 发送测试数据
 *   2. 接收上位机数据后回显
 *
 * 硬件连接：
 *   - USB DM: PA11 (AF10)
 *   - USB DP: PA12 (AF10)
 */
#ifndef USB_CDC_TEST_H
#define USB_CDC_TEST_H
#include <stdint.h>
#include <stdbool.h>
#include "../task/task.h"
#include "../driver/usb/usb_cdc.h"

#define GET_USB_CDC_TEST_VTABLE(obj) GET_TASK_VTABLE(obj)
#define GET_USB_CDC_TEST(obj) ((UsbCdcTest *)obj)

/* 派生类声明 */
typedef struct _UsbCdcTest UsbCdcTest;
typedef struct _UsbCdcTestFun UsbCdcTestFun;

struct _UsbCdcTestFun {
    void (*destroy)(UsbCdcTest* self);
};

struct _UsbCdcTest {
    Task base;
    const UsbCdcTestFun* fun;
    Usb_cdc *usb;             /* USB CDC 设备句柄 */
    uint32_t send_count;      /* 发送计数 */
    uint32_t recv_count;      /* 接收计数 */
    char tx_buffer[128];
    char rx_buffer[256];
    char echo_buf[300];
};

/* 构造函数声明 */
UsbCdcTest* usb_cdc_test_create(const task_into_t *info);
void usb_cdc_test_init(UsbCdcTest* self, const task_into_t *info);

/* 析构函数声明 */
void usb_cdc_test_deinit(UsbCdcTest* self);

#endif /* USB_CDC_TEST_H */

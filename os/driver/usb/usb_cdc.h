/**
 * usb_cdc.h — USB CDC ACM 设备类驱动
 *
 * 组合 usb_device 总线层 + CDC 类协议。
 * 不直接访问 USB 硬件寄存器。
 */
#ifndef USB_CDC_H
#define USB_CDC_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../hal/hal_usb.h"
#include "usb_device.h"
#include "../../scheduler/semaphore.h"

#define GET_USB_CDC(obj) ((Usb_cdc *)obj)

/* ── 引脚 ── */
typedef struct {
    pin_af usb_dm;
    pin_af usb_dp;
} usb_pins_t;

/* ── CDC 配置 ── */
typedef struct {
    usb_pins_t  pins;
    /* 自定义描述符（NULL 则使用内置默认值） */
    const uint8_t *device_desc;
    const uint8_t *config_desc;
    const char    *manufacturer_str;
    const char    *product_str;
    const char    *serial_str;
} usb_cdc_config_t;

/* ── 传输状态 ── */
typedef struct {
    const uint8_t  *tx_buf;
    uint8_t        *rx_buf;
    volatile uint16_t tx_len;
    volatile uint16_t rx_len;
    volatile uint16_t rx_wr_idx;
    Semaphore *usb_tx_sem;
    Semaphore *usb_rx_sem;
} usb_cdc_xfer_t;

/* ── 类声明 ── */
typedef struct _Usb_cdc Usb_cdc;
typedef struct _Usb_cdcFun Usb_cdcFun;

struct _Usb_cdcFun { void (*destroy)(Usb_cdc* self); };

struct _Usb_cdc {
    Device              base;
    const Usb_cdcFun*   fun;
    usb_device_t        usb_dev;          /* 总线层 */
    const usb_cdc_config_t *conf;
    usb_cdc_xfer_t     *usb_cdc_xfer;
};

/* ── 构造 ── */
Usb_cdc* usb_cdc_create(const device_info_t *info);
void usb_cdc_init(Usb_cdc* self, const device_info_t *info);
void usb_cdc_deinit(Usb_cdc* self);

/* ── CDC 数据收发 ── */
int usb_cdc_send(Usb_cdc* self, const uint8_t *data, uint16_t len);
int usb_cdc_recv(Usb_cdc* self, uint8_t *buffer, uint16_t len);

#endif

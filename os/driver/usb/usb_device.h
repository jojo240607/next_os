/**
 * usb_device.h — USB Device 总线驱动层
 *
 * 职责：USB 设备状态机、标准请求处理、描述符管理、端点分配。
 * 不包含任何 USB 设备类（CDC/HID/MSC）协议。
 * 上层类驱动通过注册描述符和端点配置使用总线。
 */
#ifndef USB_DEVICE_H
#define USB_DEVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "../hal/hal_usb.h"

/* ── 描述符注册 ── */
/* ── USB 标准请求码（bRequest） ── */
typedef enum : uint8_t {
    USB_REQ_GET_STATUS        = 0x00,
    USB_REQ_CLEAR_FEATURE     = 0x01,
    USB_REQ_SET_FEATURE       = 0x03,
    USB_REQ_SET_ADDRESS       = 0x05,
    USB_REQ_GET_DESCRIPTOR    = 0x06,
    USB_REQ_SET_DESCRIPTOR    = 0x07,
    USB_REQ_GET_CONFIGURATION = 0x08,
    USB_REQ_SET_CONFIGURATION = 0x09,
} usb_standard_request_t;

/* ── USB 描述符类型（wValue 高字节） ── */
typedef enum : uint8_t {
    USB_DESC_DEVICE          = 0x01,
    USB_DESC_CONFIGURATION   = 0x02,
    USB_DESC_STRING          = 0x03,
    USB_DESC_INTERFACE       = 0x04,
    USB_DESC_ENDPOINT        = 0x05,
    USB_DESC_DEVICE_QUALIFIER = 0x06,
    USB_DESC_IAD             = 0x0B,  /* Interface Association */
} usb_descriptor_type_t;

/* ── USB 标准字符串描述符索引（wValue 低字节） ── */
typedef enum : uint8_t {
    USB_STR_IDX_LANGID       = 0,
    USB_STR_IDX_MANUFACTURER = 1,
    USB_STR_IDX_PRODUCT      = 2,
    USB_STR_IDX_SERIAL       = 3,
} usb_string_index_t;

typedef struct {
    const uint8_t *device_desc;       /* 设备描述符 */
    uint16_t       device_desc_len;
    const uint8_t *config_desc;       /* 配置描述符 */
    uint16_t       config_desc_len;
    const char    *manufacturer_str;
    const char    *product_str;
    const char    *serial_str;
} usb_device_descriptors_t;

/* ── USB 设备总线状态 ── */
typedef struct {
    usb_device_state_t   state;
    uint8_t              address;
    usb_device_descriptors_t desc;
    bool                 initialized;
} usb_device_t;

/* ── 总线 API ── */

/** 初始化 USB Device 总线硬件 */
void usb_device_init(usb_device_t *dev, const usb_device_descriptors_t *desc);

/** 连接 / 断开 */
void usb_device_connect(usb_device_t *dev);
void usb_device_disconnect(usb_device_t *dev);

/** 处理 SETUP 包（在 IRQ 中调用） */
void usb_device_handle_setup(usb_device_t *dev, uint16_t rx_buf_size);

/** 查询状态 */
static inline bool usb_device_is_configured(usb_device_t *dev)
{ return dev->state == USB_STATE_CONFIGURED; }

#endif

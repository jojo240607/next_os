/**
 * usb_device.c — USB Device 总线驱动
 *
 * 处理："USB 标准请求"（第 9 章）
 * GET_DESCRIPTOR / SET_ADDRESS / SET_CONFIGURATION / GET_STATUS
 *
 * 不依赖任何设备类协议（CDC/HID/MSC）。
 */
#include "usb_device.h"
#include "string.h"

void usb_device_init(usb_device_t *dev, const usb_device_descriptors_t *desc)
{
    dev->state       = USB_STATE_DEFAULT;
    dev->address     = 0;
    dev->desc        = *desc;
    dev->initialized = true;
}

void usb_device_connect(usb_device_t *dev)
{
    (void)dev;
    hal_usb_connect();
}

void usb_device_disconnect(usb_device_t *dev)
{
    dev->state = USB_STATE_RESET;
    hal_usb_disconnect();
}

/* ── SETUP 数据通路 ── */
static void rearm_ep0_out(void) {
    xUSB_OTG_FS->DOEPTSIZ0 = (3 << 19) | 64;
    xUSB_OTG_FS->DOEPCTL0 |= xUSB_OTG_DOEPCTL_CNAK | xUSB_OTG_DOEPCTL_EPENA;
}

static void send_zlp(void)
{
    hal_usb_set_ep_tx_size(0, 1, 0);
    hal_usb_ep_tx_enable(0);
    rearm_ep0_out();
}

static void send_data_ep0(const uint8_t *data, uint16_t len)
{
    hal_usb_write_txfifo(0, data, len);
    hal_usb_set_ep_tx_size(0, 1, len);
    hal_usb_ep_tx_enable(0);
    rearm_ep0_out();
}

/* ── 标准请求处理 ── */
void usb_device_handle_setup(usb_device_t *dev, uint16_t rx_buf_size)
{
    uint8_t setup[8];
    hal_usb_read_rxfifo(setup, 8);

    uint8_t  bmRequestType = setup[0];
    uint8_t  bRequest      = setup[1];
    uint16_t wValue        = setup[2] | (setup[3] << 8);
    uint16_t wLength       = setup[6] | (setup[7] << 8);

    if ((bmRequestType & 0x60) != 0x00) {
        /* 非标准请求 → STALL，由上层 CDC 等类驱动自行处理 */
        hal_usb_ep_rx_stall(0);
        return;
    }

    switch ((usb_standard_request_t)bRequest) {

    case USB_REQ_GET_STATUS: {
        uint8_t status[] = { 0x00, 0x00 };
        send_data_ep0(status, 2);
        break;
    }
    case USB_REQ_SET_ADDRESS: {
        uint8_t addr = wValue & 0x7F;
        hal_usb_set_address(addr);
        dev->address = addr;
        dev->state   = (addr == 0) ? USB_STATE_DEFAULT : USB_STATE_ADDRESS;
        send_zlp();
        break;
    }
    case USB_REQ_GET_DESCRIPTOR: {
        usb_descriptor_type_t desc_type = (usb_descriptor_type_t)((wValue >> 8) & 0xFF);
        const uint8_t *desc = NULL;
        uint16_t desc_len   = 0;

        switch (desc_type) {
        case USB_DESC_DEVICE:
            desc = dev->desc.device_desc;
            desc_len = dev->desc.device_desc_len;
            break;
        case USB_DESC_CONFIGURATION:
            desc = dev->desc.config_desc;
            desc_len = dev->desc.config_desc_len;
            break;
        case USB_DESC_STRING: {
            switch ((usb_string_index_t)(wValue & 0xFF)) {
            case USB_STR_IDX_LANGID: {
                static const uint8_t lang_id[] = { 0x04, 0x03, 0x09, 0x04 };
                desc = lang_id; desc_len = 4; break;
            }
            case USB_STR_IDX_MANUFACTURER:
                desc = (const uint8_t *)dev->desc.manufacturer_str;
                desc_len = (uint16_t)strlen(dev->desc.manufacturer_str); break;
            case USB_STR_IDX_PRODUCT:
                desc = (const uint8_t *)dev->desc.product_str;
                desc_len = (uint16_t)strlen(dev->desc.product_str); break;
            case USB_STR_IDX_SERIAL:
                desc = (const uint8_t *)dev->desc.serial_str;
                desc_len = (uint16_t)strlen(dev->desc.serial_str); break;
            }
            break;
        }
        }
        if (desc && desc_len) {
            if (desc_len > wLength) desc_len = wLength;
            send_data_ep0(desc, desc_len);
        }
        break;
    }
    case USB_REQ_SET_CONFIGURATION: {
        if (wValue == 1) {
            dev->state = USB_STATE_CONFIGURED;
            /* 初始化 CDC 数据端点（由上层类驱动负责）:
             *   此处在总线层面只发 ZLP。
             *   上层类驱动需在 SET_CONFIGURATION 后初始化 EP1/EP2。 */
            send_zlp();
        }
        break;
    }
    default:
        hal_usb_ep_rx_stall(0);
        break;
    }
}

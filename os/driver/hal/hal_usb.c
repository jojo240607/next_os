//
// Created by zhiwei.gong on 2026/5/15.
//
#include "stddef.h"
#include "hal_usb.h"
#include "string.h"

/* ───────── 内部缓冲区 ───────── */
//static uint8_t  tx_buf[256];
//static uint8_t  rx_buf[256];
//static uint16_t tx_buf_size;
//static uint16_t rx_buf_size;
//static volatile uint16_t tx_len = 0;
//static volatile uint16_t rx_len = 0;
//static volatile uint16_t rx_rd_idx = 0;
//static volatile uint16_t rx_wr_idx = 0;

//static usb_callback_t usb_user_cb;
static usb_device_state_t usb_state = USB_STATE_RESET;
//static uint8_t usb_address = 0;


/* ───────── 标准设备描述符 ───────── */
static const uint8_t usb_device_desc[] = {
        0x12,       /* bLength */
        0x01,       /* bDescriptorType (Device) */
        0x00, 0x02, /* bcdUSB 2.00 */
        0x02,       /* bDeviceClass (CDC) */
        0x00,       /* bDeviceSubClass */
        0x00,       /* bDeviceProtocol */
        64,         /* bMaxPacketSize0 */
        0x83, 0x04, /* idVendor (0x0483 STMicro) */
        0x40, 0x57, /* idProduct (0x5740) */
        0x00, 0x02, /* bcdDevice 2.00 */
        0x01,       /* iManufacturer */
        0x02,       /* iProduct */
        0x03,       /* iSerialNumber */
        0x01        /* bNumConfigurations */
};

/* 简化：这里只给出骨架，完整 CDC 描述符约 75 字节，
   包括 IAD、CDC 接口描述符、端点描述符等。
   实际使用时需要根据 USB CDC 规范拼装完整描述符。
   下面用占位符表示： */
static const uint8_t usb_cdc_config_desc[] = {
        /* 配置描述符 (9 bytes) */
        0x09, 0x02, 0x4B, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,
        /* IAD (8 bytes) */
        0x08, 0x0B, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,
        /* CDC 接口 1 (9 bytes) */
        0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
        /* Header 功能描述符 (5 bytes) */
        0x05, 0x24, 0x00, 0x10, 0x01,
        /* ACM 功能描述符 (4 bytes) */
        0x04, 0x24, 0x02, 0x02,
        /* Union 功能描述符 (5 bytes) */
        0x05, 0x24, 0x06, 0x00, 0x01,
        /* Call Management 功能描述符 (5 bytes) */
        0x05, 0x24, 0x01, 0x00, 0x01,
        /* CDC 接口端点 (7 bytes) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0xFF,
        /* 数据接口 (9 bytes) */
        0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
        /* 数据端点 IN (7 bytes) */
        0x07, 0x05, 0x81, 0x02, 64, 0x00, 0x01,
        /* 数据端点 OUT (7 bytes) */
        0x07, 0x05, 0x01, 0x02, 64, 0x00, 0x01,
};
/* ───────── 字符串描述符 ───────── */
static const uint8_t usb_lang_id[] = { 0x04, 0x03, 0x09, 0x04 };
//static uint8_t usb_str_manufacturer[64];
//static uint8_t usb_str_product[64];
//static uint8_t usb_str_serial[64];

/* ───────── 复位 FIFO ───────── */
void usb_flush_rx_fifo(void)
{
    xUSB_OTG_FS->GRSTCTL |= xUSB_OTG_GRSTCTL_RXFFLSH;
    while (xUSB_OTG_FS->GRSTCTL & xUSB_OTG_GRSTCTL_RXFFLSH);
}

static void usb_flush_tx_fifo(uint8_t num)
{
    xUSB_OTG_FS->GRSTCTL = (num << xUSB_OTG_GRSTCTL_TXFNUM_Pos) |
                          xUSB_OTG_GRSTCTL_TXFFLSH;
    while (xUSB_OTG_FS->GRSTCTL & xUSB_OTG_GRSTCTL_TXFFLSH);
}

/* ───────── 从 RxFIFO 读取数据 ───────── */
uint16_t usb_read_rxfifo(uint8_t *buf, uint16_t len)
{
    uint16_t words = (len + 3) / 4;
    volatile uint32_t *fifo = xUSB_OTG_FS_FIFO(0);
    for (uint16_t i = 0; i < words; i++) {
        uint32_t w = *fifo;
        for (int j = 0; j < 4 && (i * 4 + j) < len; j++) {
            buf[i * 4 + j] = (w >> (j * 8)) & 0xFF;
        }
    }
    return len;
}

/* ───────── 向 TxFIFO 写入数据 ───────── */
uint16_t usb_write_txfifo(uint8_t ep, const uint8_t *buf, uint16_t len)
{
    uint16_t words = (len + 3) / 4;
    volatile uint32_t *fifo = xUSB_OTG_FS_FIFO(ep);
    for (uint16_t i = 0; i < words; i++) {
        uint32_t w = 0;
        for (int j = 0; j < 4 && (i * 4 + j) < len; j++) {
            w |= (uint32_t)(buf[i * 4 + j]) << (j * 8);
        }
        *fifo = w;
    }
    return len;
}

/* ───────── 设置设备地址 ───────── */
void usb_set_address(uint8_t addr)
{
    //usb_address = addr;
    xUSB_OTG_FS->DCFG &= ~(0x7F << 4);
    xUSB_OTG_FS->DCFG |= (addr << 4);
}

void usb_set_state(usb_device_state_t state) {
    usb_state = state;
}


/* ───────── 标准请求处理 ───────── */
void usb_handle_setup(const char *manufacturer_str, const char *product_str, const char *serial_str,uint16_t rx_buf_size)
{
    uint8_t setup[8];
    usb_read_rxfifo(setup, 8);

    uint8_t bmRequestType = setup[0];
    uint8_t bRequest      = setup[1];
    uint16_t wValue       = setup[2] | (setup[3] << 8);
    uint16_t wLength      = setup[6] | (setup[7] << 8);

    /* ─── 标准设备请求 ─── */
    if ((bmRequestType & 0x60) == 0x00) {   /* 标准请求 */
        switch (bRequest) {
            case 0x00: /* GET_STATUS (Device) */
            {
                uint8_t status[] = { 0x00, 0x00 };
                usb_write_txfifo(0, status, 2);
                xUSB_OTG_FS->DIEPTSIZ[0] = (1 << 19) | 2; /* 1 pkt, 2 bytes */
                xUSB_OTG_FS->DIEPCTL[0] |= xUSB_OTG_DIEPCTL_CNAK |
                                          xUSB_OTG_DIEPCTL_EPENA;
            }
                break;

            case 0x05: /* SET_ADDRESS */
                usb_set_address(wValue);
                /* 回复 0 长度状态包 */
                xUSB_OTG_FS->DIEPTSIZ[0] = (1 << 19);
                xUSB_OTG_FS->DIEPCTL[0] |= xUSB_OTG_DIEPCTL_CNAK |
                                          xUSB_OTG_DIEPCTL_EPENA;
                usb_state = (wValue == 0) ? USB_STATE_DEFAULT :
                            USB_STATE_ADDRESS;
                break;

            case 0x06: /* GET_DESCRIPTOR */
            {
                uint8_t desc_type = (wValue >> 8) & 0xFF;
                const uint8_t *desc = NULL;
                uint16_t desc_len = 0;

                switch (desc_type) {
                    case 0x01: /* Device Descriptor */
                        desc = usb_device_desc;
                        desc_len = sizeof(usb_device_desc);
                        break;
                    case 0x02: /* Configuration Descriptor */
                        desc = usb_cdc_config_desc;
                        desc_len = sizeof(usb_cdc_config_desc);
                        break;
                    case 0x03: /* String Descriptor */
                        switch (wValue & 0xFF) {
                            case 0:
                                desc = usb_lang_id;
                                desc_len = 4;
                                break;
                            case 1:
                                desc = (const uint8_t *)manufacturer_str;
                                desc_len = strlen(manufacturer_str);
                                break;
                            case 2:
                                desc = (const uint8_t *)product_str;
                                desc_len = strlen(product_str);
                                break;
                            case 3:
                                desc = (const uint8_t *)serial_str;
                                desc_len = strlen(serial_str);
                                break;
                        }
                        break;
                }

                if (desc && desc_len > 0) {
                    if (desc_len > wLength) desc_len = wLength;
                    usb_write_txfifo(0, desc, desc_len);
                    xUSB_OTG_FS->DIEPTSIZ[0] = (1 << 19) | desc_len;
                    xUSB_OTG_FS->DIEPCTL[0] |= xUSB_OTG_DIEPCTL_CNAK |
                                              xUSB_OTG_DIEPCTL_EPENA;
                }
            }
                break;

            case 0x09: /* SET_CONFIGURATION */
                if (wValue == 1) {
                    usb_state = USB_STATE_CONFIGURED;
                    /* 这里应初始化 CDC 数据端点 (EP1 IN/OUT) 和中断端点 */
                    /* ── 数据 IN 端点 ── */
                    xUSB_OTG_FS->DIEPCTL[1] = 0;
                    xUSB_OTG_FS->DIEPCTL[1] = (64 << 0) | (1 << 22);
                    xUSB_OTG_FS->DIEPTSIZ[1] = 0;
                    xUSB_OTG_FS->DIEPCTL[1] |= xUSB_OTG_DIEPCTL_SNAK;

                    /* ── 数据 OUT 端点 ── */
                    xUSB_OTG_FS->DOEPCTL[1] = 0;
                    xUSB_OTG_FS->DOEPCTL[1] = (64 << 0);
                    xUSB_OTG_FS->DOEPTSIZ[1] = (1 << 19) | rx_buf_size;
                    xUSB_OTG_FS->DOEPCTL[1] |= xUSB_OTG_DIEPCTL_CNAK |
                                              xUSB_OTG_DIEPCTL_EPENA;

                    /* ── 中断 IN 端点 ── */
                    xUSB_OTG_FS->DIEPCTL[2] = 0;
                    xUSB_OTG_FS->DIEPCTL[2] = (8 << 0) | (1 << 22);
                    xUSB_OTG_FS->DIEPTSIZ[2] = 0;
                    xUSB_OTG_FS->DIEPCTL[2] |= xUSB_OTG_DIEPCTL_SNAK;

                    /* 回复 0 长度状态包 */
                    xUSB_OTG_FS->DIEPTSIZ[0] = (1 << 19);
                    xUSB_OTG_FS->DIEPCTL[0] |= xUSB_OTG_DIEPCTL_CNAK |
                                              xUSB_OTG_DIEPCTL_EPENA;

                    //if (usb_user_cb)
                    //    usb_user_cb(USB_EVT_ENUM_DONE, 0, 0);
                }
                break;

            default:
                /* 未支持的请求 → STALL */
                xUSB_OTG_FS->DOEPCTL[0] |= (1 << 21);  /* STALL */
                break;
        }
    }

    /* 用户回调 */
    //if (usb_user_cb)
    //    usb_user_cb(USB_EVT_SETUP, 0, 0);
}





bool usb_cdc_is_connected(void)
{
    return (usb_state == USB_STATE_CONFIGURED);
}

void usb_cdc_deinit(void)
{
    xUSB_OTG_FS->DCTL |= xUSB_OTG_DCTL_SDIS;
    usb_state = USB_STATE_RESET;
}

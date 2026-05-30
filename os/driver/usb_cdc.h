#ifndef USB_CDC_H
#define USB_CDC_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "hal/hal_usb.h"
#include "common/pinmux.h"

#define GET_USB_CDC_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(Usb_cdcVTable **)obj)
#define GET_USB_CDC(obj) ((Usb_cdc *)obj)

// 派生类声明
typedef struct _Usb_cdc Usb_cdc;
typedef struct _Usb_cdcFun Usb_cdcFun;
// 类成员函数结构
struct _Usb_cdcFun {
    void (*destroy)(Usb_cdc* self);
};

typedef struct {
    pin_af usb_dm;
    pin_af usb_dp;
} usb_pins_t;

/* CDC 配置描述符 */
typedef struct {
    usb_pins_t pins;
    uint16_t    vendor_id;
    uint16_t    product_id;
    const char *manufacturer_str;
    const char *product_str;
    const char *serial_str;

    /* 收发缓冲区大小 */
    //uint16_t    tx_buf_size;
    //uint16_t    rx_buf_size;

    /* 中断 */
   // usb_callback_t callback;

} usb_cdc_config_t;

typedef struct {
    /* ───────── 内部缓冲区 ───────── */
    const uint8_t  *tx_buf;
    uint8_t  *rx_buf;
    //uint16_t tx_buf_size;
    //uint16_t rx_buf_size;
    volatile uint16_t tx_len;
    volatile uint16_t rx_len;
    //volatile uint16_t rx_rd_idx;
    volatile uint16_t rx_wr_idx;
    Semaphore * usb_tx_sem;
    Semaphore * usb_rx_sem;
} usb_cdc_xfer_t;
struct _Usb_cdc {
    Device base;  // 基类作为第一个成员
    const Usb_cdcFun* fun;
    // TODO: 添加派生类特有的数据成员
    const usb_cdc_config_t *conf;
    usb_cdc_xfer_t *usb_cdc_xfer;
};

// 构造函数声明
Usb_cdc* usb_cdc_create(const usb_cdc_config_t *conf, const dev_pripority_t *priority);
void usb_cdc_init(Usb_cdc* self, const usb_cdc_config_t *conf, const dev_pripority_t *priority);

// 析构函数声明
void usb_cdc_deinit(Usb_cdc* self);

#endif // USB_CDC_H
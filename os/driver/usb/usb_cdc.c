/**
 * usb_cdc.c — USB CDC ACM 设备类驱动
 */
#include "usb_cdc.h"
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include <string.h>

/* ── OOP ── */
dev_init_override(usb_cdc_dev_init_impl);
dev_read_override(usb_cdc_dev_read_impl);
dev_write_override(usb_cdc_dev_write_impl);
dev_ioctl_override(usb_cdc_ioctl_impl);
static void usb_cdc_destroy(Usb_cdc* self);
static bool usb_cdc_irq_handler(nvic_irq_t *irq_conf);

/* ── CDC ACM 默认描述符 ── */
static const uint8_t cdc_default_device_desc[] = {
    0x12, 0x01, 0x00, 0x02, 0x02, 0x00, 0x00, 0x40,
    0x83, 0x04, 0x40, 0x57, 0x00, 0x02, 0x01, 0x02, 0x03, 0x01
};
static const uint8_t cdc_default_config_desc[] = {
    /* Config */      0x09, 0x02, 0x4B, 0x00, 0x02, 0x01, 0x00, 0xC0, 0x32,
    /* IAD */         0x08, 0x0B, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,
    /* CDC Comm */    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* Header */      0x05, 0x24, 0x00, 0x10, 0x01,
    /* ACM */         0x05, 0x24, 0x01, 0x00, 0x01,
    /* Union */       0x05, 0x24, 0x02, 0x00, 0x01,
    /* Call Mgmt */   0x05, 0x24, 0x06, 0x00, 0x01,
    /* CDC Data */    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* EP1 OUT */     0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,
    /* EP1 IN */      0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,
    /* EP2 IN */      0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x10,
};

static const Usb_cdcFun usb_cdc_fun = {
    .destroy = usb_cdc_destroy,
};

/* ── create → init（匹配 usart 模式：create 中调 init 分配 vtable）── */
Usb_cdc* usb_cdc_create(const device_info_t *info) {
    if (!info) return NULL;
    Usb_cdc* self = os_malloc(sizeof(Usb_cdc));
    if (self) { memset(self, 0, sizeof(Usb_cdc)); usb_cdc_init(self, info); }
    return self;
}

void usb_cdc_init(Usb_cdc* self, const device_info_t *info) {
    device_init(&self->base, info);   /* 分配 vtable + 初始化 base */
    self->fun = &usb_cdc_fun;

    GET_DEVICE_VTABLE(self)->dev_init  = usb_cdc_dev_init_impl;
    GET_DEVICE_VTABLE(self)->dev_read  = usb_cdc_dev_read_impl;
    GET_DEVICE_VTABLE(self)->dev_write = usb_cdc_dev_write_impl;
    GET_DEVICE_VTABLE(self)->dev_ioctl = usb_cdc_ioctl_impl;
}

void usb_cdc_deinit(Usb_cdc* self) {
    device_deinit(&self->base);
}

/* ── device init ── */
dev_init_override(usb_cdc_dev_init_impl) {
    Usb_cdc *usb_cdc = (Usb_cdc *)self;
    const device_info_t *info = self->info;
    const usb_cdc_config_t *conf = (const usb_cdc_config_t *)info->conf;

    /* ── 0. 传输上下文 ── */
    usb_cdc->usb_cdc_xfer = os_malloc(sizeof(usb_cdc_xfer_t));
    memset(usb_cdc->usb_cdc_xfer, 0, sizeof(usb_cdc_xfer_t));
    usb_cdc->usb_cdc_xfer->usb_tx_sem = semaphore_create(0);
    usb_cdc->usb_cdc_xfer->usb_rx_sem = semaphore_create(0);

    /* ── 1. pinmux ── */
    const pin_config_t pins[2] = {
        { .port = AF_REQ_GET_PORT(conf->pins.usb_dm),
          .pin  = AF_REQ_GET_PIN(conf->pins.usb_dm),
          .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP,
          .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP,
          .af   = conf->pins.usb_dm },
        { .port = AF_REQ_GET_PORT(conf->pins.usb_dp),
          .pin  = AF_REQ_GET_PIN(conf->pins.usb_dp),
          .mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP,
          .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_PULLUP,
          .af   = conf->pins.usb_dp },
    };
    if (pinmux_request_group(pins, 2) != PINMUX_SUCCESS) {
        LOG_ERROR("usb_cdc", "init pinmux error!");
        return;
    }

    /* ── 2. 硬件初始化 ── */
    hal_usb_clock_enable();
    hal_usb_core_reset();
    hal_usb_set_device_mode();
    hal_usb_config_fifo(128, 64, 128, 0);
    hal_usb_config_ep0(64);
    hal_usb_enable_interrupts();

    /* ── 3. 总线层 ── */
    usb_device_descriptors_t usb_desc = {
        .device_desc       = conf->device_desc ? conf->device_desc : cdc_default_device_desc,
        .device_desc_len   = conf->device_desc ? 0 : sizeof(cdc_default_device_desc),
        .config_desc       = conf->config_desc ? conf->config_desc : cdc_default_config_desc,
        .config_desc_len   = conf->config_desc ? 0 : sizeof(cdc_default_config_desc),
        .manufacturer_str  = conf->manufacturer_str ? conf->manufacturer_str : "STM32F4",
        .product_str       = conf->product_str      ? conf->product_str      : "USB CDC",
        .serial_str        = conf->serial_str       ? conf->serial_str       : "0001",
    };
    usb_device_init(&usb_cdc->usb_dev, &usb_desc);

    /* ── 4. NVIC ── */
    self->irq_conf->handler  = usb_cdc_irq_handler;
    self->irq_conf->arg      = self;
    self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, OTG_FS_IRQ);
    self->fun->config_irq(self, self->irq_conf);

    /* ── 5. 连接 ── */
    usb_device_connect(&usb_cdc->usb_dev);
    LOG_ERROR("usb_cdc", "init dctl=%08lX gusb=%08lX", xUSB_OTG_FS->DCTL, xUSB_OTG_FS->GUSBCFG);
}

/* ═══════════════ IRQ ═══════════════ */

static void cdc_handle_set_configuration(Usb_cdc *usb_cdc, uint16_t rx_buf_size) {
    hal_usb_ep_in_init(1, 64, 2);
    hal_usb_ep_out_init(1, 64);
    hal_usb_ep_rx_enable(1, 1, rx_buf_size);
    hal_usb_ep_in_init(2, 8, 3);
}

static bool usb_cdc_irq_handler(nvic_irq_t *irq_conf) {
    Usb_cdc *usb_cdc = (Usb_cdc *)irq_conf->arg;
    usb_cdc_xfer_t *x = usb_cdc->usb_cdc_xfer;
    uint32_t gintsts = hal_usb_get_gintsts();

    /* ── USBRST ── */
    if (gintsts & xUSB_OTG_GINTSTS_USBRST) {
        hal_usb_clear_gintsts(xUSB_OTG_GINTSTS_USBRST);
        usb_cdc->usb_dev.state = USB_STATE_DEFAULT;
        hal_usb_set_address(0);
        hal_usb_flush_rx_fifo();
        xUSB_OTG_FS->DCTL &= ~xUSB_OTG_DCTL_SDIS;
        xUSB_OTG_FS->DOEPTSIZ0 = (3 << 19) | 64;
        xUSB_OTG_FS->DOEPCTL0  = xUSB_OTG_DOEPCTL_USBAEP |
                                   xUSB_OTG_DOEPCTL_CNAK |
                                   xUSB_OTG_DOEPCTL_EPENA;
        xUSB_OTG_FS->DIEPCTL0  = xUSB_OTG_DIEPCTL_USBAEP | (1 << 28);
        xUSB_OTG_FS->GINTMSK  = xUSB_OTG_GINTSTS_USBRST |
                                xUSB_OTG_GINTSTS_ENUMDNE |
                                xUSB_OTG_GINTSTS_RXFLVL |
                                xUSB_OTG_GINTSTS_OEPINT |
                                xUSB_OTG_GINTSTS_IEPINT;
        LOG_ERROR("usb_cdc", "RST");
    }

    /* ── ENUMDNE → 激活 EP0 ── */
    if (gintsts & xUSB_OTG_GINTSTS_ENUMDNE) {
        hal_usb_clear_gintsts(xUSB_OTG_GINTSTS_ENUMDNE);
        xUSB_OTG_FS->DOEPTSIZ0 = (3 << 19) | 64;
        xUSB_OTG_FS->DOEPCTL0  = xUSB_OTG_DOEPCTL_USBAEP |
                                   xUSB_OTG_DOEPCTL_CNAK |
                                   xUSB_OTG_DOEPCTL_EPENA;
        xUSB_OTG_FS->DIEPCTL0  = xUSB_OTG_DIEPCTL_USBAEP |
                                   (0 << 22) |    /* TXFNUM=0 */
                                   (1 << 28);     /* SETD0PID=1 */
    }

    /* ── Rx FIFO ── */
    if (gintsts & xUSB_OTG_GINTSTS_RXFLVL) {
        uint32_t rxst = hal_usb_get_rxst();
        uint8_t  pid  = (rxst >> 17) & 0x0F;
        uint16_t bcnt = (rxst >> 4) & 0x7FF;

        switch (pid) {
        case 0x06: /* SETUP */
            usb_device_handle_setup(&usb_cdc->usb_dev, x ? x->rx_len : 64);
            if (usb_device_is_configured(&usb_cdc->usb_dev))
                cdc_handle_set_configuration(usb_cdc, x ? x->rx_len : 64);
            break;
        case 0x02: /* OUT */
            if (x && x->rx_wr_idx < x->rx_len) {
                hal_usb_read_rxfifo(x->rx_buf + x->rx_wr_idx, bcnt);
                x->rx_wr_idx += bcnt;
                x->usb_rx_sem->fun->give(x->usb_rx_sem);
            } else {
                uint8_t dummy[64]; hal_usb_read_rxfifo(dummy, bcnt);
            }
            hal_usb_ep_rx_enable(1, 1, x ? x->rx_len : 64);
            break;
        default:
            hal_usb_read_rxfifo(NULL, 0);
            break;
        }
    }

    /* ── IN EP ── */
    if (gintsts & xUSB_OTG_GINTSTS_IEPINT) {
        if (xUSB_OTG_FS->DAINT & 1)
            xUSB_OTG_FS->DIEPINT0 = 0xFF;
    }

    /* ── OUT EP ── */
    if (gintsts & xUSB_OTG_GINTSTS_OEPINT) {
        if (xUSB_OTG_FS->DAINT & (1 << 16)) {
            uint32_t oep = xUSB_OTG_FS->DOEPINT0;
            if (oep & (1 << 3)) {  /* STUP: 主动读 SETUP */
                xUSB_OTG_FS->DOEPINT0 = (1 << 3);
                if (xUSB_OTG_FS->GINTSTS & xUSB_OTG_GINTSTS_RXFLVL) {
                    uint32_t rxst = hal_usb_get_rxst();
                    if (((rxst >> 17) & 0x0F) == 6) {
                        usb_device_handle_setup(&usb_cdc->usb_dev, x ? x->rx_len : 64);
                        if (usb_device_is_configured(&usb_cdc->usb_dev))
                            cdc_handle_set_configuration(usb_cdc, x ? x->rx_len : 64);
                    }
                }
            }
            xUSB_OTG_FS->DOEPINT0 = 0xFF;
        }
    }

    return true;
}

/* ── destroy ── */
static void usb_cdc_destroy(Usb_cdc* self) {
    if (self->usb_cdc_xfer) {
        if (self->usb_cdc_xfer->usb_tx_sem) semaphore_deinit(self->usb_cdc_xfer->usb_tx_sem);
        if (self->usb_cdc_xfer->usb_rx_sem) semaphore_deinit(self->usb_cdc_xfer->usb_rx_sem);
        os_free(self->usb_cdc_xfer);
    }
    os_free(self);
}

/* ── Device VTable ── */
dev_read_override(usb_cdc_dev_read_impl) {
    return usb_cdc_recv((Usb_cdc *)self, (uint8_t *)buf, (uint16_t)count);
}
dev_write_override(usb_cdc_dev_write_impl) {
     usb_cdc_send((Usb_cdc *)self, (const uint8_t *)buf, (uint16_t)count);
}
dev_ioctl_override(usb_cdc_ioctl_impl) {
    (void)self; (void)cmd; (void)arg;
}

/* ── CDC 数据收发 ── */
int usb_cdc_send(Usb_cdc* self, const uint8_t *data, uint16_t len) {
    usb_cdc_xfer_t *x = self->usb_cdc_xfer;
    if (!x || !len) return -1;
    x->tx_buf = data;
    x->tx_len = len;
    hal_usb_ep_tx_enable(1);
    x->usb_tx_sem->fun->take(x->usb_tx_sem);
    return x->tx_len;
}

int usb_cdc_recv(Usb_cdc* self, uint8_t *buffer, uint16_t len) {
    usb_cdc_xfer_t *x = self->usb_cdc_xfer;
    if (!x) return -1;
    x->rx_buf = buffer;
    x->rx_len = len;
    x->rx_wr_idx = 0;
    x->usb_rx_sem->fun->take(x->usb_rx_sem);
    return x->rx_wr_idx;
}

#include "usb_cdc.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "common/rcc.h"

dev_init_override(usb_cdc_dev_init_impl);

// 析构函数声明
static void usb_cdc_destroy(Usb_cdc* self);
static bool usb_cdc_irq_handler_impl(nvic_irq_t *irq_conf);

// TODO: 初始化数据成员
static const Usb_cdcFun usb_cdc_fun = {
    .destroy = usb_cdc_destroy,
};



// 构造函数实现
Usb_cdc* usb_cdc_create(const usb_cdc_config_t *conf, const dev_pripority_t *priority) {
    Usb_cdc* obj = (Usb_cdc*)os_malloc(sizeof(Usb_cdc));
    if (obj) {
        memset(obj, 0, sizeof(Usb_cdc));
        usb_cdc_init(obj, conf, priority);
    }
    return obj;
}

void usb_cdc_init(Usb_cdc* self, const usb_cdc_config_t *conf, const dev_pripority_t *priority) {
    // 初始化基类部分
    device_init(&self->base, priority);
    self->fun = &(usb_cdc_fun);
    // TODO: 初始化派生类特有成员

	def_dev_init(self) = usb_cdc_dev_init_impl;
    self->conf = conf;
    self->usb_cdc_xfer = NULL;

}

void usb_cdc_deinit(Usb_cdc* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void usb_cdc_destroy(Usb_cdc* self) {
    if (self != NULL) {
        usb_cdc_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(usb_cdc_dev_init_impl) {
    // TODO: add dev_init method
    Usb_cdc *usb_cdc = (Usb_cdc *)self;
    //params 
    if (!usb_cdc->conf) {
        return;
    }

    /* 1. 使能 OTG FS 时钟 (AHB2 bit7) */
    rcc_periph_clock_enable(RCC_BUS_AHB2, xRCC_AHB2ENR_OTGFSEN);

    /* 2. 配置引脚 */
    pin_config_t usb_pins[] = {
            { .mode = PIN_MODE_AF,
              .otype = PIN_OTYPE_PP,
              .ospeed = PIN_OSPEED_HIGH,
              .pupd = PIN_PUPD_NONE, .af =  usb_cdc->conf->pins.usb_dm},//PA11_REQ_OTG_FS_DM
            { .mode = PIN_MODE_AF,
              .otype = PIN_OTYPE_PP,
              .ospeed = PIN_OSPEED_HIGH,
              .pupd = PIN_PUPD_NONE, .af =  usb_cdc->conf->pins.usb_dp},//PA12_REQ_OTG_FS_DP
    };
    pinmux_request_group(usb_pins, 2);

    /* 3. 保存配置 */
    //usb_user_cb  = usb_cdc->conf->callback;
    //tx_buf_size  = usb_cdc->conf->tx_buf_size;
   // rx_buf_size  = usb_cdc->conf->rx_buf_size;

    /* 4. 软复位 */
    xUSB_OTG_FS->GAHBCFG = xUSB_OTG_GAHBCFG_GINT;
    xUSB_OTG_FS->GUSBCFG |= xUSB_OTG_GUSBCFG_FDMOD;
    xUSB_OTG_FS->GRSTCTL = xUSB_OTG_GRSTCTL_CSRST;
    while (xUSB_OTG_FS->GRSTCTL & xUSB_OTG_GRSTCTL_CSRST);
    for (volatile int i = 0; i < 100000; i++);

    /* 5. 强制 Device 模式, 设置 TRDT */
    xUSB_OTG_FS->GUSBCFG = xUSB_OTG_GUSBCFG_FDMOD | (9 << xUSB_OTG_GUSBCFG_TRDT_Pos);

    /* 6. 配置 FIFO */
      xUSB_OTG_FS->GRXFSIZ    = 128;   /* 128 words = 512 bytes */
      xUSB_OTG_FS->DIEPTXF[0] = (128 << 16) | 128;  /* 128 words for EP0 Tx */
      xUSB_OTG_FS->DIEPTXF[1] = (256 << 16) | 128;  /* 128 words for EP1 Tx */
      xUSB_OTG_FS->DIEPTXF[2] = (384 << 16) | 32;   /* 32 words for EP2 Tx */

    /* 7. 配置控制端点 0 */
      xUSB_OTG_FS->DIEPCTL[0] = (64 << 0);
      xUSB_OTG_FS->DOEPCTL[0] = (64 << 0);
      xUSB_OTG_FS->DOEPTSIZ[0] = (3 << 19) | 0;
      xUSB_OTG_FS->DOEPCTL[0] |= xUSB_OTG_DIEPCTL_CNAK | xUSB_OTG_DIEPCTL_EPENA;

    /* 8. 使能中断 */
      xUSB_OTG_FS->DIEPMSK = xUSB_OTG_DIEPINT_XFRC | xUSB_OTG_DIEPINT_TOC;
      xUSB_OTG_FS->DOEPMSK = xUSB_OTG_DOEPINT_XFRC | xUSB_OTG_DOEPINT_STPKTRX;
      xUSB_OTG_FS->DAINTMSK = (1 << 0) | (1 << 16);
      xUSB_OTG_FS->GINTMSK = xUSB_OTG_GINTSTS_USBRST |
                          xUSB_OTG_GINTSTS_ENUMDNE |
                          xUSB_OTG_GINTSTS_OEPINT |
                          xUSB_OTG_GINTSTS_IEPINT |
                          xUSB_OTG_GINTSTS_RXFLVL;


    /* 9. NVIC */
    //nvic_set_priority(OTG_FS_IRQn, 0, 0);
    //nvic_enable_irq(OTG_FS_IRQn);
    //self->irq_conf.priority = self->fun->encode_pripority(self, 0x00, 0x00);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
    self->irq_conf->handler = usb_cdc_irq_handler_impl;
    self->irq_conf->arg = self;
    self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, OTG_FS_IRQ);
    self->fun->config_irq(self, self->irq_conf);

    if (!usb_cdc->usb_cdc_xfer) {
        usb_cdc->usb_cdc_xfer = os_malloc(sizeof(usb_cdc_xfer_t));
        usb_cdc->usb_cdc_xfer->tx_len = 0;
        usb_cdc->usb_cdc_xfer->rx_len = 0;
        // usb_cdc->usb_cdc_xfer.rx_rd_idx = 0;
        usb_cdc->usb_cdc_xfer->rx_wr_idx = 0;
        usb_cdc->usb_cdc_xfer->usb_rx_sem = semaphore_create(0);
        usb_cdc->usb_cdc_xfer->usb_tx_sem = semaphore_create(0);
    }
    /* 10. 上电 */
      xUSB_OTG_FS->DCTL &= ~xUSB_OTG_DCTL_SDIS;  /* 清除 Soft Disconnect */
}

static bool usb_cdc_irq_handler_impl(nvic_irq_t *irq_conf) {
    Usb_cdc * usb_cdc = (Usb_cdc *)irq_conf->arg;
    uint32_t gintsts = xUSB_OTG_FS->GINTSTS;

    /* ── 复位中断 ── */
    if (gintsts & xUSB_OTG_GINTSTS_USBRST) {
        xUSB_OTG_FS->GINTSTS = xUSB_OTG_GINTSTS_USBRST;

        /* 复位设备模式寄存器 */
        xUSB_OTG_FS->DCFG |= xUSB_OTG_DCFG_DSPD;
        xUSB_OTG_FS->DCTL &= ~xUSB_OTG_DCTL_SDIS;
        usb_set_address(0);
        usb_set_state(USB_STATE_DEFAULT);

        /* 清 Rx FIFO */
        usb_flush_rx_fifo();

        //if (usb_user_cb) usb_user_cb(USB_EVT_RESET, 0, 0);
    }

    /* ── 枚举完成 ── */
    if (gintsts & xUSB_OTG_GINTSTS_ENUMDNE) {
        xUSB_OTG_FS->GINTSTS = xUSB_OTG_GINTSTS_ENUMDNE;
        //if (usb_user_cb) usb_user_cb(USB_EVT_ENUM_DONE, 0, 0);
    }
    usb_cdc_xfer_t *x = usb_cdc->usb_cdc_xfer;
    if (!x) {
        return true;
    }
    /* ── Rx FIFO 非空 ── */
    if (gintsts & xUSB_OTG_GINTSTS_RXFLVL) {
        uint32_t rxst = xUSB_OTG_FS->GRXSTSP;
        uint8_t ep = rxst & 0x0F;
        uint16_t bcnt = (rxst >> 4) & 0x7FF;

        switch ((rxst >> 17) & 0x0F) {
            case 0x06: /* SETUP 包 */
                usb_handle_setup(usb_cdc->conf->manufacturer_str, usb_cdc->conf->product_str,
                                 usb_cdc->conf->serial_str, x->rx_len);
                break;
            case 0x02: /* OUT 数据包 */
                if (x->rx_wr_idx < x->rx_len) {
                    usb_read_rxfifo(x->rx_buf + x->rx_wr_idx, bcnt);
                    x->rx_wr_idx += bcnt;
                    x->usb_rx_sem->fun->give(x->usb_rx_sem);
                } else {
                    // 缓冲区满，直接丢弃数据并读取 FIFO 清空硬件
                    uint8_t dummy[64];
                    usb_read_rxfifo(dummy, bcnt);
                }
                //if (usb_user_cb)
                //    usb_user_cb(USB_EVT_RX_READY, ep, bcnt);
                /* 重新使能 OUT 端点 */
                xUSB_OTG_FS->DOEPTSIZ[1] = (1 << 19) | x->rx_len;
                xUSB_OTG_FS->DOEPCTL[1] |= xUSB_OTG_DIEPCTL_CNAK |
                                          xUSB_OTG_DIEPCTL_EPENA;

                break;
        }
    }

    /* ── IN 端点中断 ── */
    if (gintsts & xUSB_OTG_GINTSTS_IEPINT) {
        uint32_t daint = xUSB_OTG_FS->DAINT;
        for (int ep = 0; ep < 4; ep++) {
            if (daint & (1 << ep)) {
                uint32_t diepint = xUSB_OTG_FS->DIEPINT[ep];
                if (diepint & xUSB_OTG_DIEPINT_XFRC) {
                    xUSB_OTG_FS->DIEPINT[ep] = xUSB_OTG_DIEPINT_XFRC;
                    x->usb_tx_sem->fun->give(x->usb_tx_sem);
                   // if (usb_user_cb)
                   //     usb_user_cb(USB_EVT_TX_DONE, ep | 0x80, tx_len);
                }
            }
        }
    }

    /* ── OUT 端点中断 ── */
    if (gintsts & xUSB_OTG_GINTSTS_OEPINT) {
        uint32_t daint = (xUSB_OTG_FS->DAINT >> 16);
        for (int ep = 0; ep < 4; ep++) {
            if (daint & (1 << ep)) {
                uint32_t doepint = xUSB_OTG_FS->DOEPINT[ep];
                if (doepint & xUSB_OTG_DOEPINT_XFRC) {
                    xUSB_OTG_FS->DOEPINT[ep] = xUSB_OTG_DOEPINT_XFRC;
                }
            }
        }
    }
}

/* ───────── 发送数据 ───────── */
int usb_cdc_send(Usb_cdc* self, const uint8_t *data, uint16_t len)
{
    if (!usb_cdc_is_connected()) {
        return -1;
    }
    usb_cdc_xfer_t *x = self->usb_cdc_xfer;
    if (!x) {
        return true;
    }
    x->tx_buf = data;
    x->tx_len = len;

    usb_write_txfifo(1, x->tx_buf, len);
    xUSB_OTG_FS->DIEPTSIZ[1] = (1 << 19) | len;
    xUSB_OTG_FS->DIEPCTL[1] |= xUSB_OTG_DIEPCTL_CNAK | xUSB_OTG_DIEPCTL_EPENA;
    x->usb_tx_sem->fun->take(x->usb_tx_sem);
    return 0;
}
//uint16_t usb_cdc_available(Usb_cdc* self)
//{
//    if (x->rx_wr_idx >= x->rx_rd_idx) {
//        return x->rx_wr_idx - x->rx_rd_idx;
//    } else {
//        return x->rx_len - x->rx_rd_idx + x->rx_wr_idx;
//    }
//}
/* ───────── 接收数据 ───────── */
int usb_cdc_recv(Usb_cdc* self, uint8_t *buffer, uint16_t len)
{
    usb_cdc_xfer_t *x = self->usb_cdc_xfer;
    if (!x) {
        return true;
    }
    x->rx_buf = buffer;
    x->rx_len = len;
    //uint16_t avail = usb_cdc_available(self);
    //if (avail == 0) {
    //    return 0;
    //}
    //if (len > avail) {
    //    len = avail;
    //}
    x->usb_rx_sem->fun->take(x->usb_rx_sem);
    return len;
}

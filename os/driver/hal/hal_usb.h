//
// Created by zhiwei.gong on 2026/5/15.
//

#ifndef STM32F4DISCOVERY_HAL_USB_H
#define STM32F4DISCOVERY_HAL_USB_H
#include "stdint.h"
#include "stdbool.h"

/* ───────── OTG FS 寄存器基址 ───────── */
#define xUSB_OTG_FS_BASE    0x50000000UL

/* ───────── 全局寄存器 ───────── */
typedef struct {
    volatile uint32_t GOTGCTL;       /* 0x000 */
    volatile uint32_t GOTGINT;       /* 0x004 */
    volatile uint32_t GAHBCFG;       /* 0x008 */
    volatile uint32_t GUSBCFG;       /* 0x00C */
    volatile uint32_t GRSTCTL;       /* 0x010 */
    volatile uint32_t GINTSTS;       /* 0x014 */
    volatile uint32_t GINTMSK;       /* 0x018 */
    volatile uint32_t GRXSTSR;       /* 0x01C */
    volatile uint32_t GRXSTSP;       /* 0x020 */
    volatile uint32_t GRXFSIZ;       /* 0x024 */
    volatile uint32_t HNPTXFSIZ;     /* 0x028 */
    volatile uint32_t DIEPTXF[15];   /* 0x02C.. */
    uint32_t reserved[185];
    volatile uint32_t HCFG;          /* 0x400 */
    volatile uint32_t HFIR;          /* 0x404 */
    uint32_t reserved2;
    volatile uint32_t HPTXFSIZ;      /* 0x40C */
    volatile uint32_t HPRT;          /* 0x440 */
    uint32_t reserved3[47];
    volatile uint32_t DCFG;          /* 0x800 */
    volatile uint32_t DCTL;          /* 0x804 */
    volatile uint32_t DSTS;          /* 0x808 */
    uint32_t reserved4;
    volatile uint32_t DIEPMSK;       /* 0x810 */
    volatile uint32_t DOEPMSK;       /* 0x814 */
    volatile uint32_t DAINT;         /* 0x818 */
    volatile uint32_t DAINTMSK;      /* 0x81C */
    uint32_t reserved5[2];
    volatile uint32_t DVBUSDIS;      /* 0x828 */
    volatile uint32_t DVBUSPULSE;    /* 0x82C */
    volatile uint32_t DIEPEMPMSK;    /* 0x834 */
    uint32_t reserved6[50];
    volatile uint32_t DIEPCTL[4];    /* 0x900.. */
    uint32_t reserved7[4];
    volatile uint32_t DIEPINT[4];    /* 0x908.. */
    uint32_t reserved8[4];
    volatile uint32_t DIEPTSIZ[4];   /* 0x910.. */
    volatile uint32_t DIEPDMA[4];    /* 0x914.. */
    uint32_t reserved9[16];
    volatile uint32_t DOEPCTL[4];    /* 0xB00.. */
    uint32_t reserved10[4];
    volatile uint32_t DOEPINT[4];    /* 0xB08.. */
    uint32_t reserved11[4];
    volatile uint32_t DOEPTSIZ[4];   /* 0xB10.. */
    volatile uint32_t DOEPDMA[4];    /* 0xB14.. */
    uint32_t reserved12[16];
    volatile uint32_t PCGCCTL;       /* 0xE00.. */
} xUSB_OTG_FS_TypeDef;

#define xUSB_OTG_FS  ((xUSB_OTG_FS_TypeDef *)xUSB_OTG_FS_BASE)

/* ───────── 数据 FIFO (以字访问) ───────── */
#define xUSB_OTG_FS_FIFO_BASE  0x50001000UL
#define xUSB_OTG_FS_FIFO(n)    ((volatile uint32_t *)(xUSB_OTG_FS_FIFO_BASE + ((n) * 0x1000UL)))

/* ───────── DCFG ───────── */
#define xUSB_OTG_DCFG_DSPD     (3 << 0)   /* Full Speed */
#define xUSB_OTG_DCFG_NZLSOHSK (1 << 2)

/* ───────── DCTL ───────── */
#define xUSB_OTG_DCTL_RWUSIG   (1 << 0)
#define xUSB_OTG_DCTL_SDIS     (1 << 1)
#define xUSB_OTG_DCTL_GINSTS   (1 << 2)
#define xUSB_OTG_DCTL_GONSTS   (1 << 3)
#define xUSB_OTG_DCTL_TCTL     (7 << 4)
#define xUSB_OTG_DCTL_SGINAK   (1 << 7)
#define xUSB_OTG_DCTL_CGINAK   (1 << 8)
#define xUSB_OTG_DCTL_SGONAK   (1 << 9)
#define xUSB_OTG_DCTL_CGONAK   (1 << 10)
#define xUSB_OTG_DCTL_POPRGDNE (1 << 11)

/* ───────── GRSTCTL ───────── */
#define xUSB_OTG_GRSTCTL_CSRST   (1 << 0)
#define xUSB_OTG_GRSTCTL_RXFFLSH (1 << 4)
#define xUSB_OTG_GRSTCTL_TXFFLSH (1 << 5)
#define xUSB_OTG_GRSTCTL_TXFNUM_Pos 6

/* ───────── GAHBCFG ───────── */
#define xUSB_OTG_GAHBCFG_GINT    (1 << 0)
#define xUSB_OTG_GAHBCFG_TXFELVL (1 << 7)
#define xUSB_OTG_GAHBCFG_PTXFELVL (1 << 8)

/* ───────── GUSBCFG ───────── */
#define xUSB_OTG_GUSBCFG_FDMOD  (1 << 30) /* Force Device Mode */
#define xUSB_OTG_GUSBCFG_TRDT_Pos 10

/* ───────── DIEPCTL ───────── */
#define xUSB_OTG_DIEPCTL_MPSIZ_Pos  0
#define xUSB_OTG_DIEPCTL_USBAEP    (1 << 15)
#define xUSB_OTG_DIEPCTL_SNAK      (1 << 27)
#define xUSB_OTG_DIEPCTL_CNAK      (1 << 26)
#define xUSB_OTG_DIEPCTL_EPENA     (1 << 31)

/* ───────── DOEPCTL ───────── */
#define xUSB_OTG_DOEPCTL_MPSIZ_Pos  0
#define xUSB_OTG_DOEPCTL_USBAEP    (1 << 15)
#define xUSB_OTG_DOEPCTL_SNAK      (1 << 27)
#define xUSB_OTG_DOEPCTL_CNAK      (1 << 26)
#define xUSB_OTG_DOEPCTL_EPENA     (1 << 31)

/* ───────── DIEPTSIZ / DOEPTSIZ ───────── */
#define xUSB_OTG_DIEPTSIZ_XFRSIZ_Pos  0
#define xUSB_OTG_DIEPTSIZ_PKTCNT_Pos  19
#define xUSB_OTG_DIEPTSIZ_MCNT_Pos    29

/* ───────── DIEPINT ───────── */
#define xUSB_OTG_DIEPINT_XFRC    (1 << 0)
#define xUSB_OTG_DIEPINT_TOC     (1 << 3)
#define xUSB_OTG_DIEPINT_EPDISD  (1 << 1)
#define xUSB_OTG_DIEPINT_TXFEMP  (1 << 7)

/* ───────── DOEPINT ───────── */
#define xUSB_OTG_DOEPINT_XFRC    (1 << 0)
#define xUSB_OTG_DOEPINT_STPKTRX (1 << 3)  /* SETUP 包接收 */
#define xUSB_OTG_DOEPINT_EPDISD  (1 << 1)

/* ───────── GINTSTS / GINTMSK ───────── */
#define xUSB_OTG_GINTSTS_CMOD     (1 << 0)
#define xUSB_OTG_GINTSTS_MMIS     (1 << 1)
#define xUSB_OTG_GINTSTS_OTGINT   (1 << 2)
#define xUSB_OTG_GINTSTS_SOF      (1 << 3)
#define xUSB_OTG_GINTSTS_RXFLVL   (1 << 4)
#define xUSB_OTG_GINTSTS_USBSUSP  (1 << 11)
#define xUSB_OTG_GINTSTS_USBRST   (1 << 12)
#define xUSB_OTG_GINTSTS_ENUMDNE  (1 << 13)
#define xUSB_OTG_GINTSTS_IEPINT   (1 << 18)
#define xUSB_OTG_GINTSTS_OEPINT   (1 << 19)
#define xUSB_OTG_GINTSTS_WKUPINT  (1 << 31)

/* ───────── 端点分配 ───────── */
#define EP0_IN          0x80  /* 控制端点 IN */
#define EP0_OUT         0x00  /* 控制端点 OUT */
#define EP_CDC_IN       0x81  /* CDC 数据 IN 端点 1 */
#define EP_CDC_OUT      0x01  /* CDC 数据 OUT 端点 1 */
#define EP_CDC_INT      0x82  /* CDC 中断 IN 端点 2 */


/* USB 状态 */
typedef enum : uint8_t {
    USB_STATE_RESET    = 0,
    USB_STATE_DEFAULT  = 1,
    USB_STATE_ADDRESS  = 2,
    USB_STATE_CONFIGURED = 3
} usb_device_state_t;

/* 中断事件 */
typedef enum : uint8_t {
    USB_EVT_RESET      = 0,
    USB_EVT_ENUM_DONE  = 1,
    USB_EVT_SETUP      = 2,
    USB_EVT_TX_DONE    = 3,
    USB_EVT_RX_READY   = 4,
    USB_EVT_ERROR      = 5
} usb_event_t;

void usb_set_address(uint8_t addr);
void usb_set_state(usb_device_state_t state);
void usb_handle_setup(const char *manufacturer_str, const char *product_str, const char *serial_str,uint16_t rx_buf_size);
uint16_t usb_read_rxfifo(uint8_t *buf, uint16_t len);
void usb_flush_rx_fifo(void);
uint16_t usb_write_txfifo(uint8_t ep, const uint8_t *buf, uint16_t len);
bool usb_cdc_is_connected(void);
#endif //STM32F4DISCOVERY_HAL_USB_H

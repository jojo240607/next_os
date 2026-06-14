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
    volatile uint32_t DIEPTXF[3];     /* 0x02C.. 设备模式 IN EP TxFIFO */
    volatile uint32_t GCCFG;          /* 0x038 通用核心配置 */
    uint32_t reserved[241];           /* 0x03C..0x3FC */  /* 0x400-0x03C=0x3C4=964→241words */
    volatile uint32_t HCFG;          /* 0x400 */
    volatile uint32_t HFIR;          /* 0x404 */
    uint32_t reserved2;
    volatile uint32_t HPTXFSIZ;      /* 0x40C */
    uint32_t _r410[12];              /* 0x410..0x43C */  /* 0x440-0x410=0x30=48→12words */
    volatile uint32_t HPRT;          /* 0x440 */
    uint32_t reserved3[239];         /* 0x444 .. 0x7FC */
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
    uint32_t _r830;                  /* 0x830 — reserved，手册要求补 4 字节 */
    volatile uint32_t DIEPEMPMSK;    /* 0x834 */
    uint32_t reserved6[50];
    /* ── EP0-IN ── */              /* 0x900 */
    volatile uint32_t DIEPCTL0;     /* 0x900 */
    uint32_t _rep0a;
    volatile uint32_t DIEPINT0;     /* 0x908 */
    uint32_t _rep0b;
    volatile uint32_t DIEPTSIZ0;    /* 0x910 */
    volatile uint32_t DIEPDMA0;     /* 0x914 */
    volatile uint32_t DTXFSTS0;     /* 0x918 */
    uint32_t _rep0c;
    /* ── EP1-IN ── */              /* 0x920 */
    volatile uint32_t DIEPCTL1;     /* 0x920 */
    uint32_t _rep1a;
    volatile uint32_t DIEPINT1;     /* 0x928 */
    uint32_t _rep1b;
    volatile uint32_t DIEPTSIZ1;    /* 0x930 */
    volatile uint32_t DIEPDMA1;     /* 0x934 */
    volatile uint32_t DTXFSTS1;     /* 0x938 */
    uint32_t _rep1c;
    /* ── EP2-IN ── */              /* 0x940 */
    volatile uint32_t DIEPCTL2;     /* 0x940 */
    uint32_t _rep2a;
    volatile uint32_t DIEPINT2;     /* 0x948 */
    uint32_t _rep2b;
    volatile uint32_t DIEPTSIZ2;    /* 0x950 */
    volatile uint32_t DIEPDMA2;     /* 0x954 */
    volatile uint32_t DTXFSTS2;     /* 0x958 */
    uint32_t _rep2c;
    /* ── EP3-IN ── */              /* 0x960 */
    volatile uint32_t DIEPCTL3;     /* 0x960 */
    uint32_t _rep3a;
    volatile uint32_t DIEPINT3;     /* 0x968 */
    uint32_t _rep3b;
    volatile uint32_t DIEPTSIZ3;    /* 0x970 */
    volatile uint32_t DIEPDMA3;     /* 0x974 */
    volatile uint32_t DTXFSTS3;     /* 0x978 */
    uint32_t _rep3c;
    uint32_t reserved7[96];         /* 0x980..0xAFC */
    /* ── EP0-OUT ── */             /* 0xB00 */
    volatile uint32_t DOEPCTL0;     /* 0xB00 */
    uint32_t _rep4a;
    volatile uint32_t DOEPINT0;     /* 0xB08 */
    uint32_t _rep4b;
    volatile uint32_t DOEPTSIZ0;    /* 0xB10 */
    volatile uint32_t DOEPDMA0;     /* 0xB14 */
    uint32_t _rep4c[2];
    /* ── EP1-OUT ── */             /* 0xB20 */
    volatile uint32_t DOEPCTL1;     /* 0xB20 */
    uint32_t _rep5a;
    volatile uint32_t DOEPINT1;     /* 0xB28 */
    uint32_t _rep5b;
    volatile uint32_t DOEPTSIZ1;    /* 0xB30 */
    volatile uint32_t DOEPDMA1;     /* 0xB34 */
    uint32_t _rep5c[2];
    /* ── EP2-OUT ── */             /* 0xB40 */
    volatile uint32_t DOEPCTL2;     /* 0xB40 */
    uint32_t _rep6a;
    volatile uint32_t DOEPINT2;     /* 0xB48 */
    uint32_t _rep6b;
    volatile uint32_t DOEPTSIZ2;    /* 0xB50 */
    volatile uint32_t DOEPDMA2;     /* 0xB54 */
    uint32_t _rep6c[2];
    /* ── EP3-OUT ── */             /* 0xB60 */
    volatile uint32_t DOEPCTL3;     /* 0xB60 */
    uint32_t _rep7a;
    volatile uint32_t DOEPINT3;     /* 0xB68 */
    uint32_t _rep7b;
    volatile uint32_t DOEPTSIZ3;    /* 0xB70 */
    volatile uint32_t DOEPDMA3;     /* 0xB74 */
    uint32_t _rep7c[2];
    uint32_t reserved8[160];        /* 0xB80..0xDFC */
    volatile uint32_t PCGCCTL;       /* 0xE00 */
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
uint16_t usb_read_rxfifo(uint8_t *buf, uint16_t len);
void usb_flush_rx_fifo(void);
uint16_t usb_write_txfifo(uint8_t ep, const uint8_t *buf, uint16_t len);

/* ═══════════════ 总线初始化 API（HAL 层） ═══════════════ */
void hal_usb_clock_enable(void);
void hal_usb_core_reset(void);
void hal_usb_set_device_mode(void);
void hal_usb_config_fifo(uint16_t rx_size, uint16_t ep0_tx, uint16_t ep1_tx, uint16_t ep2_tx);
void hal_usb_config_ep0(uint16_t mps);
void hal_usb_enable_interrupts(void);
void hal_usb_connect(void);
void hal_usb_disconnect(void);

/* EP 控制 */
void hal_usb_ep_in_init(uint8_t ep, uint16_t mps, uint8_t type);
void hal_usb_ep_out_init(uint8_t ep, uint16_t mps);
void hal_usb_ep_tx_enable(uint8_t ep);
void hal_usb_ep_tx_stall(uint8_t ep);
void hal_usb_ep_rx_enable(uint8_t ep, uint16_t pktcnt, uint16_t xfrsiz);
void hal_usb_ep_rx_stall(uint8_t ep);
void hal_usb_set_ep_tx_size(uint8_t ep, uint16_t pktcnt, uint16_t xfrsiz);

/* FIFO */
void hal_usb_flush_rx_fifo(void);
void hal_usb_flush_tx_fifo(uint8_t num);
uint16_t hal_usb_read_rxfifo(uint8_t *buf, uint16_t len);
uint16_t hal_usb_write_txfifo(uint8_t ep, const uint8_t *buf, uint16_t len);

/* 寄存器读取 */
uint32_t hal_usb_get_gintsts(void);
void hal_usb_clear_gintsts(uint32_t m);
uint32_t hal_usb_get_daint(void);
uint32_t hal_usb_get_diepint(uint8_t ep);
uint32_t hal_usb_get_doepint(uint8_t ep);
void hal_usb_clear_diepint(uint8_t ep, uint32_t v);
void hal_usb_clear_doepint(uint8_t ep, uint32_t v);
uint32_t hal_usb_get_rxst(void);

/* 兼容旧 API */
void hal_usb_set_address(uint8_t addr);

/* ── 调试：打印 USB 核心关键寄存器 ── */
void hal_usb_debug_dump_regs(void);

#endif //STM32F4DISCOVERY_HAL_USB_H

/**
 * hal_usb.c — USB OTG FS HAL 层（纯寄存器操作）
 * 不包含任何 USB 协议逻辑（标准请求、描述符等）。
 */
#include "stddef.h"
#include "hal_usb.h"
#include "string.h"
#include "../common/rcc.h"
#include "../../log/log.h"

/* ── 端点寄存器指针映射（结构体已改为 per-EP 显式成员，通过 switch 索引）── */
static inline volatile uint32_t* _diepctl(uint8_t ep) {
    switch(ep) {
        case 0: return &xUSB_OTG_FS->DIEPCTL0; case 1: return &xUSB_OTG_FS->DIEPCTL1;
        case 2: return &xUSB_OTG_FS->DIEPCTL2; case 3: return &xUSB_OTG_FS->DIEPCTL3;
        default: return NULL;
    }
}
static inline volatile uint32_t* _dieptsiz(uint8_t ep) {
    switch(ep) {
        case 0: return &xUSB_OTG_FS->DIEPTSIZ0; case 1: return &xUSB_OTG_FS->DIEPTSIZ1;
        case 2: return &xUSB_OTG_FS->DIEPTSIZ2; case 3: return &xUSB_OTG_FS->DIEPTSIZ3;
        default: return NULL;
    }
}
static inline volatile uint32_t* _diepint(uint8_t ep) {
    switch(ep) {
        case 0: return &xUSB_OTG_FS->DIEPINT0; case 1: return &xUSB_OTG_FS->DIEPINT1;
        case 2: return &xUSB_OTG_FS->DIEPINT2; case 3: return &xUSB_OTG_FS->DIEPINT3;
        default: return NULL;
    }
}
static inline volatile uint32_t* _doepctl(uint8_t ep) {
    switch(ep) {
        case 0: return &xUSB_OTG_FS->DOEPCTL0; case 1: return &xUSB_OTG_FS->DOEPCTL1;
        case 2: return &xUSB_OTG_FS->DOEPCTL2; case 3: return &xUSB_OTG_FS->DOEPCTL3;
        default: return NULL;
    }
}
static inline volatile uint32_t* _doeptsiz(uint8_t ep) {
    switch(ep) {
        case 0: return &xUSB_OTG_FS->DOEPTSIZ0; case 1: return &xUSB_OTG_FS->DOEPTSIZ1;
        case 2: return &xUSB_OTG_FS->DOEPTSIZ2; case 3: return &xUSB_OTG_FS->DOEPTSIZ3;
        default: return NULL;
    }
}
static inline volatile uint32_t* _doepint(uint8_t ep) {
    switch(ep) {
        case 0: return &xUSB_OTG_FS->DOEPINT0; case 1: return &xUSB_OTG_FS->DOEPINT1;
        case 2: return &xUSB_OTG_FS->DOEPINT2; case 3: return &xUSB_OTG_FS->DOEPINT3;
        default: return NULL;
    }
}

/* ═══════════════ 总线硬件初始化 ═══════════════ */

void hal_usb_clock_enable(void)
{
    rcc_periph_clock_enable(RCC_BUS_AHB2, xRCC_AHB2ENR_OTGFSEN);

    /* 硬件复位 OTG FS 外设（通过 RCC AHB2 复位寄存器 bit7） */
    *(volatile uint32_t *)0x40023814 |= (1 << 7);    /* OTGFSRST = 1 */
    for (volatile int i = 0; i < 1000; i++) __NOP();
    *(volatile uint32_t *)0x40023814 &= ~(1 << 7);   /* OTGFSRST = 0 */
    for (volatile int i = 0; i < 1000; i++) __NOP();
}

void hal_usb_core_reset(void)
{
    xUSB_OTG_FS->GAHBCFG  = xUSB_OTG_GAHBCFG_GINT;
    xUSB_OTG_FS->GUSBCFG  = xUSB_OTG_GUSBCFG_FDMOD | (9 << 10) | 0x40;
    xUSB_OTG_FS->GOTGCTL |= (1 << 19) | (1 << 18);
    xUSB_OTG_FS->DCTL    |= xUSB_OTG_DCTL_SDIS;
    xUSB_OTG_FS->GCCFG    = (1 << 21) | (1 << 19) | (1 << 18);
    for (volatile int _d = 0; _d < 2000000; _d++) __NOP();
    xUSB_OTG_FS->GINTSTS  = 0xBFFFFFFF;
}

void hal_usb_set_device_mode(void)
{
    xUSB_OTG_FS->DCFG = xUSB_OTG_DCFG_DSPD;
    xUSB_OTG_FS->PCGCCTL = 0;
}

void hal_usb_config_fifo(uint16_t rx_size, uint16_t ep0_tx_size,
                          uint16_t ep1_tx_size, uint16_t ep2_tx_size)
{
    xUSB_OTG_FS->GRXFSIZ    = rx_size;
    xUSB_OTG_FS->DIEPTXF[0] = (rx_size << 16) | ep0_tx_size;
    if (ep1_tx_size)
        xUSB_OTG_FS->DIEPTXF[1] = ((rx_size + ep0_tx_size) << 16) | ep1_tx_size;
    if (ep2_tx_size)
        xUSB_OTG_FS->DIEPTXF[2] = ((rx_size + ep0_tx_size + ep1_tx_size) << 16) | ep2_tx_size;
}

void hal_usb_config_ep0(uint16_t mps)
{
    /* 只设 MPS，不使能（EPENA 在 ENUMDNE 中设，匹配 Demo） */
    xUSB_OTG_FS->DIEPCTL0 = (mps << 0);
    xUSB_OTG_FS->DOEPCTL0 = (mps << 0) | xUSB_OTG_DOEPCTL_USBAEP;
    xUSB_OTG_FS->DOEPTSIZ0 = (3 << 19) | 0;  /* STUPCNT=3 */
}

void hal_usb_enable_interrupts(void)
{
    xUSB_OTG_FS->GAHBCFG |= xUSB_OTG_GAHBCFG_GINT;  /* 全局中断使能（CSRST 会清零） */
    xUSB_OTG_FS->DIEPMSK  = xUSB_OTG_DIEPINT_XFRC | xUSB_OTG_DIEPINT_TOC;
    xUSB_OTG_FS->DOEPMSK  = xUSB_OTG_DOEPINT_XFRC | xUSB_OTG_DOEPINT_STPKTRX;
    xUSB_OTG_FS->DAINTMSK = (1 << 0) | (1 << 16);
    xUSB_OTG_FS->GINTMSK  = xUSB_OTG_GINTSTS_USBRST |
                            xUSB_OTG_GINTSTS_ENUMDNE |
                            xUSB_OTG_GINTSTS_OEPINT |
                            xUSB_OTG_GINTSTS_IEPINT |
                            xUSB_OTG_GINTSTS_RXFLVL;
}

void hal_usb_connect(void)
{
    xUSB_OTG_FS->DCTL &= ~xUSB_OTG_DCTL_SDIS;
}

void hal_usb_disconnect(void)
{
    xUSB_OTG_FS->DCTL |= xUSB_OTG_DCTL_SDIS;
}

/* ═══════════════ FIFO 操作 ═══════════════ */

void hal_usb_flush_rx_fifo(void)
{
    xUSB_OTG_FS->GRSTCTL |= xUSB_OTG_GRSTCTL_RXFFLSH;
    while (xUSB_OTG_FS->GRSTCTL & xUSB_OTG_GRSTCTL_RXFFLSH);
}

void hal_usb_flush_tx_fifo(uint8_t num)
{
    xUSB_OTG_FS->GRSTCTL = (num << xUSB_OTG_GRSTCTL_TXFNUM_Pos) |
                           xUSB_OTG_GRSTCTL_TXFFLSH;
    while (xUSB_OTG_FS->GRSTCTL & xUSB_OTG_GRSTCTL_TXFFLSH);
}

uint16_t hal_usb_read_rxfifo(uint8_t *buf, uint16_t len)
{
    uint16_t words = (len + 3) / 4;
    volatile uint32_t *fifo = xUSB_OTG_FS_FIFO(0);
    for (uint16_t i = 0; i < words; i++) {
        uint32_t w = *fifo;
        for (int j = 0; j < 4 && (i * 4 + j) < len; j++)
            buf[i * 4 + j] = (w >> (j * 8)) & 0xFF;
    }
    return len;
}

uint16_t hal_usb_write_txfifo(uint8_t ep, const uint8_t *buf, uint16_t len)
{
    uint16_t words = (len + 3) / 4;
    volatile uint32_t *fifo = xUSB_OTG_FS_FIFO(ep);
    for (uint16_t i = 0; i < words; i++) {
        uint32_t w = 0;
        for (int j = 0; j < 4 && (i * 4 + j) < len; j++)
            w |= (uint32_t)(buf[i * 4 + j]) << (j * 8);
        *fifo = w;
    }
    return len;
}

/* ═══════════════ 地址 / 状态 / EP 控制 ═══════════════ */

void hal_usb_set_address(uint8_t addr)
{
    xUSB_OTG_FS->DCFG &= ~(0x7F << 4);
    xUSB_OTG_FS->DCFG |= (addr << 4);
}

void hal_usb_set_ep_tx_size(uint8_t ep, uint16_t pktcnt, uint16_t xfrsiz)
{
    *_dieptsiz(ep) = ((uint32_t)pktcnt << 19) | xfrsiz;
}

void hal_usb_ep_tx_enable(uint8_t ep)
{
    *_diepctl(ep) |= xUSB_OTG_DIEPCTL_CNAK | xUSB_OTG_DIEPCTL_EPENA;
}

void hal_usb_ep_tx_stall(uint8_t ep)
{
    *_diepctl(ep) |= (1 << 21);
}

void hal_usb_ep_rx_enable(uint8_t ep, uint16_t pktcnt, uint16_t xfrsiz)
{
    *_doeptsiz(ep) = ((uint32_t)pktcnt << 19) | xfrsiz;
    *_doepctl(ep) |= xUSB_OTG_DIEPCTL_CNAK | xUSB_OTG_DIEPCTL_EPENA;
}

void hal_usb_ep_rx_stall(uint8_t ep)
{
    *_doepctl(ep) |= (1 << 21);
}

void hal_usb_ep_in_init(uint8_t ep, uint16_t mps, uint8_t type)
{
    *_diepctl(ep) = 0;
    *_diepctl(ep) = (mps << 0) | (type << 18) | (1 << 22);
    *_dieptsiz(ep) = 0;
    *_diepctl(ep) |= xUSB_OTG_DIEPCTL_SNAK;
}

void hal_usb_ep_out_init(uint8_t ep, uint16_t mps)
{
    *_doepctl(ep) = 0;
    *_doepctl(ep) = (mps << 0);
}

uint32_t hal_usb_get_gintsts(void)     { return xUSB_OTG_FS->GINTSTS; }
void hal_usb_clear_gintsts(uint32_t m) { xUSB_OTG_FS->GINTSTS = m; }
uint32_t hal_usb_get_daint(void)       { return xUSB_OTG_FS->DAINT; }
uint32_t hal_usb_get_diepint(uint8_t ep)  { return *_diepint(ep); }
uint32_t hal_usb_get_doepint(uint8_t ep)  { return *_doepint(ep); }
void hal_usb_clear_diepint(uint8_t ep, uint32_t v) { *_diepint(ep) = v; }
void hal_usb_clear_doepint(uint8_t ep, uint32_t v) { *_doepint(ep) = v; }
uint32_t hal_usb_get_rxst(void)        { return xUSB_OTG_FS->GRXSTSP; }

/* ───────── 兼容旧 API ───────── */
void usb_set_address(uint8_t addr)     { hal_usb_set_address(addr); }
void usb_flush_rx_fifo(void)           { hal_usb_flush_rx_fifo(); }
uint16_t usb_read_rxfifo(uint8_t *b, uint16_t l) { return hal_usb_read_rxfifo(b,l); }
uint16_t usb_write_txfifo(uint8_t e, const uint8_t *b, uint16_t l) { return hal_usb_write_txfifo(e,b,l); }

/* ═══════════════ 调试 ═══════════════ */

void hal_usb_debug_dump_regs(void)
{
    LOG_DEBUG("usb",
        "USB: GAHBCFG=0x%08lX GUSBCFG=0x%08lX GCCFG=0x%08lX\r\n",
        xUSB_OTG_FS->GAHBCFG, xUSB_OTG_FS->GUSBCFG, xUSB_OTG_FS->GCCFG);
    LOG_DEBUG("usb",
        "USB: GINTSTS=0x%08lX GINTMSK=0x%08lX\r\n",
        xUSB_OTG_FS->GINTSTS, xUSB_OTG_FS->GINTMSK);
    LOG_DEBUG("usb",
        "USB: DCFG=0x%08lX DCTL=0x%08lX DSTS=0x%08lX\r\n",
        xUSB_OTG_FS->DCFG, xUSB_OTG_FS->DCTL, xUSB_OTG_FS->DSTS);
    LOG_DEBUG("usb",
        "USB: DIEPCTL0=0x%08lX DOEPCTL0=0x%08lX\r\n",
        xUSB_OTG_FS->DIEPCTL0, xUSB_OTG_FS->DOEPCTL0);
    LOG_DEBUG("usb",
        "USB: GRXFSIZ=0x%08lX DIEPTXF0=0x%08lX\r\n",
        xUSB_OTG_FS->GRXFSIZ, xUSB_OTG_FS->DIEPTXF[0]);
}

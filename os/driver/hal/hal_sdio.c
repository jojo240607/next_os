#include "hal_sdio.h"
#include "../common/rcc.h"

void hal_sdio_clock_enable() {
    rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB1ENR_SDIOEN);
}

void hal_sdio_power_on(void) {
    xSDIO->POWER = xSDIO_POWER_PWRCTRL_ON;
    for (volatile int i = 0; i < 10000; i++);
}

uint32_t hal_sdio_build_clkcr(uint8_t clock_div, uint8_t bus_width,
                              uint8_t clock_edge, uint8_t power_save, uint8_t hw_flow) {
    uint32_t clkcr = 0;
    if (clock_edge)      clkcr |= (1UL << 12);
    if (power_save)      clkcr |= (1UL << 11);
    if (hw_flow)         clkcr |= (1UL << 14);
    if (bus_width == 1)  clkcr |= (1UL << 10);
    clkcr |= (clock_div & 0xFF) << 0;
    clkcr |= (1UL << 8);
    return clkcr;
}

void hal_sdio_config_clock(uint32_t clkcr) { xSDIO->CLKCR = clkcr; }
void hal_sdio_set_arg(uint32_t arg)          { xSDIO->ARG = arg; }

uint32_t hal_sdio_build_cmd(uint32_t cmd_index, uint32_t resp_type) {
    /* resp_type 取低 2 位，映射到 CMD 寄存器 WAITRESP (bit6-7):
     *   0 → 00 (No response)
     *   1 → 01 (Short, 对应 SDIO_RESPONSE_SHORT)
     *   2 → 10 (Short no CRC)
     *   3 → 11 (Long,  对应 SDIO_RESPONSE_LONG) */
    return (cmd_index & 0x3F) | ((resp_type & 0x3) << 6) | xSDIO_CMD_CPSMEN;
}

void hal_sdio_send_cmd(uint32_t cmd_reg) { xSDIO->CMD = cmd_reg; }
uint32_t hal_sdio_get_sta(void)          { return xSDIO->STA; }
void hal_sdio_clear_icr(uint32_t mask)   { xSDIO->ICR = mask; }
uint32_t hal_sdio_get_resp(int index)    { return xSDIO->RESP[index]; }

bool hal_sdio_sta_has_cmd_done(uint32_t sta) {
    return (sta & (xSDIO_STA_CMDREND | xSDIO_STA_CMDSENT)) != 0;
}

bool hal_sdio_sta_has_error(uint32_t sta) {
    return (sta & (xSDIO_STA_CCRCFAIL | xSDIO_STA_CTIMEOUT | xSDIO_STA_DCRCFAIL |
                   xSDIO_STA_DTIMEOUT | xSDIO_STA_TXUNDERR | xSDIO_STA_RXOVERR)) != 0;
}

void hal_sdio_set_dlen(uint32_t len)      { xSDIO->DLEN = len; }
void hal_sdio_set_dtimer(uint32_t timer)  { xSDIO->DTIMER = timer; }
void hal_sdio_set_dctrl(uint32_t dctrl)   { xSDIO->DCTRL = dctrl; }
uint32_t hal_sdio_read_fifo(void)         { return xSDIO->FIFO; }
void hal_sdio_write_fifo(uint32_t data)   { xSDIO->FIFO = data; }
void hal_sdio_enable_dma(void)            { xSDIO->DCTRL |= (1UL << 3); }
void hal_sdio_set_mask(uint32_t mask)     { xSDIO->MASK = mask; }
uint32_t hal_sdio_get_fifo_addr(void)     { return (uint32_t)&xSDIO->FIFO; }

bool hal_sdio_sta_check_dataend(void)     { return (xSDIO->STA & xSDIO_STA_DATAEND) != 0; }
bool hal_sdio_sta_check_txfifohe(void)    { return (xSDIO->STA & (1UL << 11)) != 0; }
bool hal_sdio_sta_check_rxfifohf(void)    { return (xSDIO->STA & (1UL << 12)) != 0; }

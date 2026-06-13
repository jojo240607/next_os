#ifndef STM32F4DISCOVERY_HAL_SDIO_H
#define STM32F4DISCOVERY_HAL_SDIO_H
#include "stdint.h"
#include <stdbool.h>

#define xSDIO_BASE  0x40012C00UL
#define xSDIO       ((xSDIO_TypeDef *)xSDIO_BASE)

#define xSDIO_POWER_PWRCTRL_ON   (0x03 << 0)
#define xSDIO_CMD_WAITRESP_SHORT  (0x01UL << 6)
#define xSDIO_CMD_WAITRESP_LONG   (0x03UL << 6)
#define xSDIO_CMD_WAITRESP_NO     (0x00UL << 6)
#define xSDIO_CMD_CPSMEN          (0x01UL << 10)
#define xSDIO_STA_CTIMEOUT  (1UL << 2)
#define xSDIO_STA_CCRCFAIL  (1UL << 0)
#define xSDIO_STA_CMDREND   (1UL << 6)
#define xSDIO_STA_CMDSENT   (1UL << 7)
#define xSDIO_STA_DTIMEOUT  (1UL << 10)
#define xSDIO_STA_DCRCFAIL  (1UL << 1)
#define xSDIO_STA_DATAEND   (1UL << 8)
#define xSDIO_STA_TXUNDERR  (1UL << 4)
#define xSDIO_STA_RXOVERR   (1UL << 3)
#define xSDIO_STA_STBITERR  (1UL << 9)

typedef struct {
    volatile uint32_t POWER;
    volatile uint32_t CLKCR;
    volatile uint32_t ARG;
    volatile uint32_t CMD;
    volatile const uint32_t RESPCMD;
    volatile const uint32_t RESP[4];
    volatile uint32_t DTIMER;
    volatile uint32_t DLEN;
    volatile uint32_t DCTRL;
    volatile const uint32_t DCOUNT;
    volatile const uint32_t STA;
    volatile uint32_t ICR;
    volatile uint32_t MASK;
    uint32_t RESERVED[2];
    volatile const uint32_t FIFOCNT;
    uint32_t RESERVED1[13];
    volatile uint32_t FIFO;
} xSDIO_TypeDef;

void hal_sdio_clock_enable(void);
void hal_sdio_power_on(void);
void hal_sdio_config_clock(uint32_t clkcr);
uint32_t hal_sdio_build_clkcr(uint8_t clock_div, uint8_t bus_width,
                              uint8_t clock_edge, uint8_t power_save, uint8_t hw_flow);
void hal_sdio_set_arg(uint32_t arg);
void hal_sdio_send_cmd(uint32_t cmd_reg);
uint32_t hal_sdio_build_cmd(uint32_t cmd_index, uint32_t resp_type);
uint32_t hal_sdio_get_sta(void);
void hal_sdio_clear_icr(uint32_t mask);
uint32_t hal_sdio_get_resp(int index);
bool hal_sdio_sta_has_cmd_done(uint32_t sta);
bool hal_sdio_sta_has_error(uint32_t sta);
void hal_sdio_set_dlen(uint32_t len);
void hal_sdio_set_dtimer(uint32_t timer);
void hal_sdio_set_dctrl(uint32_t dctrl);
uint32_t hal_sdio_read_fifo(void);
void hal_sdio_write_fifo(uint32_t data);
void hal_sdio_enable_dma(void);
void hal_sdio_set_mask(uint32_t mask);
uint32_t hal_sdio_get_fifo_addr(void);
bool hal_sdio_sta_check_dataend(void);
bool hal_sdio_sta_check_txfifohe(void);
bool hal_sdio_sta_check_rxfifohf(void);

#endif //STM32F4DISCOVERY_HAL_SDIO_H

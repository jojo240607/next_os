//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_SDIO_H
#define STM32F4DISCOVERY_HAL_SDIO_H
#include "stdint.h"




#define xSDIO_BASE  0x40012C00UL
#define xSDIO       ((xSDIO_TypeDef *)xSDIO_BASE)

/* ---------- SDIO 电源控制 ---------- */
#define xSDIO_POWER_PWRCTRL_ON   (0x03 << 0)   /* 上电 */

/* ---------- SDIO 命令寄存器位 ---------- */
#define xSDIO_CMD_CMDINDEX_SHIFT  0
#define xSDIO_CMD_CMDINDEX_MASK   (0x3FUL << xSDIO_CMD_CMDINDEX_SHIFT)
#define xSDIO_CMD_RESPONSE_SHIFT  6
#define xSDIO_CMD_RESPONSE_MASK   (0x03UL << xSDIO_CMD_RESPONSE_SHIFT)
#define xSDIO_CMD_WAITRESP_SHORT  (0x01UL << 6)
#define xSDIO_CMD_WAITRESP_LONG   (0x03UL << 6)
#define xSDIO_CMD_WAITRESP_NO     (0x00UL << 6)
#define xSDIO_CMD_WAITINT         (0x01UL << 8)
#define xSDIO_CMD_WAITPEND        (0x01UL << 9)
#define xSDIO_CMD_CPSMEN          (0x01UL << 10)

/* ---------- SDIO 状态位 ---------- */
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

/* ---------- SDIO 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t POWER;           /* 电源控制 */
    volatile uint32_t CLKCR;           /* 时钟控制 */
    volatile uint32_t ARG;             /* 命令参数 */
    volatile uint32_t CMD;             /* 命令寄存器  */
    volatile const uint32_t RESPCMD;   /* 响应命令   */
    volatile const uint32_t RESP[4];   /* 响应寄存器 */
    volatile uint32_t DTIMER;          /* 数据超时   */
    volatile uint32_t DLEN;            /* 数据长度   */
    volatile uint32_t DCTRL;           /* 数据控制   */
    volatile const uint32_t DCOUNT;    /* 数据计数   */
    volatile const uint32_t STA;       /* 状态寄存器 */
    volatile uint32_t ICR;             /* 中断清除   */
    volatile uint32_t MASK;            /* 中断屏蔽   */
    uint32_t RESERVED[2];
    volatile const uint32_t FIFOCNT;   /* FIFO计数器 */
    uint32_t RESERVED1[13];
    volatile uint32_t FIFO;            /* 数据FIFO   */
} xSDIO_TypeDef;

void hal_sdio_clock_enable();
#endif //STM32F4DISCOVERY_HAL_SDIO_H

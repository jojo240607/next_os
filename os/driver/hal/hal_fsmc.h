#ifndef STM32F4DISCOVERY_HAL_FSMC_H
#define STM32F4DISCOVERY_HAL_FSMC_H
#include "stdint.h"

/* FSMC 寄存器基址 */
#define xFSMC_BASE 0xA0000000UL
typedef struct {
    volatile uint32_t BCR;
    volatile uint32_t BTR;
    volatile uint32_t BWTR;
} xFSMC_Bank1_TypeDef;
#define xFSMC_Bank1 ((xFSMC_Bank1_TypeDef *)xFSMC_BASE)

/* FSMC Bank1 Region1 映射基址 (NE1) */
#define FSMC_BANK1_BASE 0x60000000UL

/* ── HAL 函数 ── */
void hal_fsmc_clock_enable(void);
void hal_fsmc_config_bank1(uint32_t bcr, uint32_t btr);

#endif

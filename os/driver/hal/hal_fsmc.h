//
// Created by zhiwei.gong on 2026/5/15.
//

#ifndef STM32F4DISCOVERY_HAL_FSMC_H
#define STM32F4DISCOVERY_HAL_FSMC_H
#include "stdint.h"


/* FSMC 寄存器基址 */
#define xFSMC_BASE 0xA0000000UL
/* FSMC Bank1 寄存器定义 */
typedef struct {
    volatile uint32_t BCR;  /* 控制寄存器 */
    volatile uint32_t BTR;  /* 时序寄存器 */
    volatile uint32_t BWTR; /* 写时序寄存器(可选) */
} xFSMC_Bank1_TypeDef;

#define xFSMC_Bank1 ((xFSMC_Bank1_TypeDef *)xFSMC_BASE)

/* 基址 (Bank1 Region1, NE1) */
#define FSMC_LCD_BASE         0x60000000UL

/* 根据选择的FSMC地址线 (0~25) 计算命令/数据地址 */
#define FSMC_LCD_CMD_ADDR(base)       (*(volatile uint16_t *)(base))
#define FSMC_LCD_DATA_ADDR(base, A)   (*(volatile uint16_t *)((base) + (2UL << (A))))

/* 示例：选择 A16 作为 RS 线 */
#define RS_ADDR_LINE (16)
#define LCD_CMD_ADDR   FSMC_LCD_CMD_ADDR(FSMC_LCD_BASE)
#define LCD_DATA_ADDR  FSMC_LCD_DATA_ADDR(FSMC_LCD_BASE, RS_ADDR_LINE)


void fsmc_lcd_write_cmd(uint8_t cmd);
void fsmc_lcd_write_data(uint8_t data);

#endif //STM32F4DISCOVERY_HAL_FSMC_H

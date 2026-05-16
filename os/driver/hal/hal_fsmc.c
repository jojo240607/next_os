//
// Created by zhiwei.gong on 2026/5/15.
//

#include "hal_fsmc.h"

/* ===================================================================
   LCD 基础操作
   =================================================================== */
void fsmc_lcd_write_cmd(uint8_t cmd) {
    LCD_CMD_ADDR = cmd;
}

void fsmc_lcd_write_data(uint8_t data) {
    LCD_DATA_ADDR = data;
}



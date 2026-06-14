//
// Created by zhiwei.gong on 2026/4/28.
//

#ifndef STM32F4DISCOVERY_DEVICE_CONFIG_H
#define STM32F4DISCOVERY_DEVICE_CONFIG_H

#include "timer.h"
#include "systick.h"
#include "usart/usart.h"
#include "timer.h"
#include "exti.h"
#include "adc/adc.h"
#include "spi/spi.h"
#include "i2c/i2c.h"
#include "can/can.h"
#include "pwm.h"
#include "i2s/i2s.h"
#include "fsmc/fsmc.h"         /* fsmc_config_t */
#include "lcd/lcd_fsmc.h"      /* lcd_fsmc_config_t */

#include "common/pinmux.h"
#include "common/dma.h"
#include "common/rcc.h"
#include "wdg.h"
#include "hal/hal_fpu.h"
#include "../device/icm20948.h"
#include "../device/adxl345.h"
#include "usb/usb_cdc.h"

extern const rcc_sysclk_config_t clk_conf;
extern const systick_config_t sys_tick_conf;
extern const usart_config_t usart4_conf;
extern const usart_config_t usart1_conf;
extern const tim_config_t time2_conf;
extern const exti_config_t exti_conf;
extern const adc_config_t adc1_conf;
extern const spi_config_t spi1_conf;
extern const i2c_config_t i2c1_conf;
extern const iwdg_config_t iwdg_conf;
extern const can_config_t can1_conf;
extern const pwm_config_t pwm1_conf;
extern const i2s_config_t i2s2_conf;
extern const fsmc_config_t fsmc_conf;
extern const lcd_fsmc_config_t lcd_fsmc_conf;
extern const icm20948_config_t icm20948_conf;
extern const adxl345_config_t adxl345_conf;
extern const usb_cdc_config_t usb_cdc_conf;

#endif //STM32F4DISCOVERY_DEVICE_CONFIG_H

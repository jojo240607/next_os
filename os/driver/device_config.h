//
// Created by zhiwei.gong on 2026/4/28.
//

#ifndef STM32F4DISCOVERY_DEVICE_CONFIG_H
#define STM32F4DISCOVERY_DEVICE_CONFIG_H

#include "timer.h"
#include "systick.h"
#include "usart.h"
#include "timer.h"
#include "exti.h"
#include "adc.h"
#include "spi.h"
#include "i2c.h"
#include "can.h"
#include "pwm.h"
#include "i2s.h"
#include "fsmc.h"

#include "common/pinmux.h"
#include "common/dma.h"
#include "common/rcc.h"
#include "wdg.h"
#include "hal/hal_fpu.h"

extern const rcc_sysclk_config_t clk_conf;
extern const systick_config_t sys_tick_conf;
extern const usart_config usart4_conf;
extern const usart_config usart1_conf;
extern const tim_config_t time2_conf;
extern const exti_config exti_conf;
extern const adc_config adc1_conf;
extern const spi_config_t spi1_conf;
extern const i2c_config_t i2c1_conf;
extern const iwdg_config_t iwdg_conf;
extern const can_config_t can1_conf;
extern const pwm_config_t pwm1_conf;
extern const i2s_config_t i2s2_conf;
extern const fsmc_lcd_config_t fsmc_conf;


#endif //STM32F4DISCOVERY_DEVICE_CONFIG_H

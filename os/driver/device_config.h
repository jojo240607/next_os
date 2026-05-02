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
#include "common/pinmux.h"

extern const systick_conf sys_tick_conf;
extern const usart_config usart4_conf;
extern const usart_config usart1_conf;
extern const timer_config time2_conf;
extern const exti_config exti_conf;

#endif //STM32F4DISCOVERY_DEVICE_CONFIG_H

//
// Created by zhiwei.gong on 2026/4/27.
//

#ifndef STM32F4DISCOVERY_SYS_TIME_H
#define STM32F4DISCOVERY_SYS_TIME_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../driver/hal/hal_dwt.h"

typedef struct sys_time_t sys_time;

extern sys_time_t *gloable_sys_time;
void systime_init();

sys_time_t *get_systime_us();

#endif //STM32F4DISCOVERY_SYS_TIME_H

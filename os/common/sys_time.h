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

typedef struct _sys_time sys_time;

struct _sys_time {
    volatile uint32_t systick;
};
extern sys_time *gloable_sys_time;
void systime_init();

uint64_t get_systime_us();

#endif //STM32F4DISCOVERY_SYS_TIME_H

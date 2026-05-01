//
// Created by zhiwei.gong on 2026/4/27.
//

#ifndef STM32F4DISCOVERY_SYS_TIME_H
#define STM32F4DISCOVERY_SYS_TIME_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
typedef struct _sys_time sys_time;

struct _sys_time {
    volatile uint32_t systick;
};
void systime_init();

extern sys_time *gloable_sys_time;
static inline sys_time *getSystime() {
    return gloable_sys_time;
}
#endif //STM32F4DISCOVERY_SYS_TIME_H

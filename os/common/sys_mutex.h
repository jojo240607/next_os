//
// Created by zhiwei.gong on 2026/4/29.
//

#ifndef STM32F4DISCOVERY_SYS_MUTEX_H
#define STM32F4DISCOVERY_SYS_MUTEX_H
#include "../scheduler/mutex.h"

extern Mutex *gloable_mutex;

void sys_mutex_init();

#endif //STM32F4DISCOVERY_SYS_MUTEX_H

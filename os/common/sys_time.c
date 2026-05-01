//
// Created by zhiwei.gong on 2026/4/27.
//

#include "sys_time.h"
#include "linear_pool.h"
#include "../log/log.h"


sys_time *gloable_sys_time;

void systime_init() {
    LOG_DEBUG("sys_time", "systime_init");
    gloable_sys_time = os_malloc(sizeof(sys_time));
}


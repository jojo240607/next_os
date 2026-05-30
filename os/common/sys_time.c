//
// Created by zhiwei.gong on 2026/4/27.
//

#include "sys_time.h"
#include "linear_pool.h"
#include "../log/log.h"
#include "../driver/svc.h"


sys_time_t *gloable_sys_time;

void systime_init() {
    LOG_DEBUG("sys_time", "systime_init");
    gloable_sys_time = os_malloc(sizeof(sys_time_t));
    memset(gloable_sys_time, 0, sizeof(sys_time_t));
    hal_dwt_init();
}


sys_time_t *get_systime_us() {
    return gloable_sys_time;
}




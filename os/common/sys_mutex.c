//
// Created by zhiwei.gong on 2026/4/29.
//

#include "sys_mutex.h"

Mutex *gloable_mutex;

void sys_mutex_init() {
    gloable_mutex = mutex_create();
}
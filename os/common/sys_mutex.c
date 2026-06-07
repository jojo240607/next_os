//
// Created by zhiwei.gong on 2026/4/29.
//

/*
 * 全局系统互斥锁 —— 供 monitor_task 等需要全局临界区的场景使用。
 * 注意：调度器内部临界区使用 arch_irq_lock/unlock (BASEPRI)，
 * 此锁主要用于任务间互斥，非中断保护。
 */
#include "sys_mutex.h"

Mutex *gloable_mutex;

void sys_mutex_init() {
    gloable_mutex = mutex_create();
}
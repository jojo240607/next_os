//
// Created by zhiwei.gong on 2026/5/22.
//

#ifndef STM32F4DISCOVERY_SVC_H
#define STM32F4DISCOVERY_SVC_H
#include "hal/hal_svc.h"
#include "../scheduler/thread_scheduler.h"
#include "../scheduler/mutex.h"
#include "common/device.h"

/* ── 用户友好的包装函数 ── */
static inline uint32_t start_pendsv_user() {
    return SVC_CALL0(SVC_PEND_SVC);
}

static inline uint32_t sleep_user(uint32_t ms) {
    return SVC_CALL1(SVC_TASK_SLEEP, ms);
}

static inline uint32_t sem_take_user(Semaphore *semaphore) {
    return SVC_CALL1(SVC_SEMAPHORE_TAKE, semaphore);
}

static inline uint32_t sem_give_user(Semaphore *semaphore) {
    return SVC_CALL1(SVC_SEMAPHORE_GIVE, semaphore);
}

static inline uint32_t mutex_lock_user(Mutex *mutex, uint32_t timeout_ms) {
    return SVC_CALL2(SVC_MUTEX_LOCK, mutex, timeout_ms);
}

static inline uint32_t mutex_unlock_user(Mutex *mutex) {
    return SVC_CALL1(SVC_MUTEX_UNLOCK, mutex);
}

static inline uint32_t device_user(const Device *device, const device_ctrl *ctrl) {
    return SVC_CALL2(SVC_DEVICE, device, ctrl);
}

static inline void task_exit_user(void) {
    SVC_CALL0(SVC_TASK_EXIT);
    while(1);
}

int system_svc_init(void);

#endif //STM32F4DISCOVERY_SVC_H

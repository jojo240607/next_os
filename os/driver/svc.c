//
// Created by zhiwei.gong on 2026/5/22.
//

#include "svc.h"
#include "../common/util.h"
#include "../scheduler/mutex.h"
#include "../log/log.h"
#include "hal/hal_dwt.h"


static uint32_t kstart_pendsv(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t ksleep(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t ksem_take(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t ksem_give(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t kmutex_lock(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t kmutex_unlock(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t kdevice(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
static uint32_t kget_systime(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);
/* 系统调用表 */
const svc_entry_t svc_entries[] = {
        { SVC_PEND_SVC, kstart_pendsv },
        { SVC_TASK_SLEEP,  ksleep },
        { SVC_SEMAPHORE_TAKE,  ksem_take },
        { SVC_SEMAPHORE_GIVE,  ksem_give },
        { SVC_MUTEX_LOCK,  kmutex_lock },
        { SVC_MUTEX_UNLOCK,  kmutex_unlock },
        { SVC_DEVICE,    kdevice },
        {SVC_GET_TIME_US, kget_systime },
        // ... 其他系统调用
};


int system_svc_init(void) {
    // 1. 硬件初始化（RCC, MPU, NVIC 等）
    // 2. 初始化 MPU（保护外设区等）
    // 3. 初始化 SVC
    svc_config_t svc_cfg = {
            .entries     = svc_entries,
            .num_entries = sizeof(svc_entries) / sizeof(svc_entries[0])
    };
    svc_init(&svc_cfg);
    return 1;
}


static uint32_t kstart_pendsv(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    Trigger_PendSV;
    return 1;
}
static uint32_t ksleep(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    gloable_current_tcb->fun->os_sleep((Tcb_t *)gloable_current_tcb, a1);
    return 1;
}


static uint32_t ksem_take(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (!a1) {
        return 0;
    }
    Semaphore *semaphore = (Semaphore *)a1;
    semaphore->fun->take(semaphore);
    return 1;
}
static uint32_t ksem_give(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (!a1) {
        return 0;
    }
    Semaphore *semaphore = (Semaphore *)a1;
    semaphore->fun->give(semaphore);
    return 1;
}

static uint32_t kmutex_lock(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (!a1) {
        return 0;
    }
    Mutex *mutex = (Mutex *)a1;
    LOG_DEBUG("svc", "kmutex_lock");
    return mutex->fun->mutex_lock(mutex, a2);
}
static uint32_t kmutex_unlock(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (!a1) {
        return 0;
    }
    Mutex *mutex = (Mutex *)a1;
    LOG_DEBUG("svc", "kmutex_unlock");
    return mutex->fun->mutex_unlock(mutex);
}

static uint32_t kdevice(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (!a1 || !a2) {
        return 0;
    }
    Device *device = (Device *)a1;
    const device_ctrl *ctrl = (const device_ctrl *)a2;
    const device_transfer_conf_t *transfer_conf = ctrl->buf;
    switch (ctrl->cmd) {
        case DEVICE_READ:
            if (device->vtable->dev_read) {
                return device->vtable->dev_read(device, ctrl->buf, ctrl->count);
            }

            break;
        case DEVICE_WRITE:
            if (device->vtable->dev_write) {
                device->vtable->dev_write(device, ctrl->buf, ctrl->count);
            }
            break;
        case DEVICE_IOCTL:
            if (device->vtable->dev_ioctl) {
                device->vtable->dev_ioctl(device, transfer_conf->cmd, transfer_conf->conf);
            }
            break;
        default:
            break;
    }
    return 1;
}

static uint32_t kget_systime(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    return (uint32_t) hal_dwt_get_timestamp_us();
}


//
// Created by zhiwei.gong on 2026/5/22.
//

#ifndef STM32F4DISCOVERY_HAL_SVC_H
#define STM32F4DISCOVERY_HAL_SVC_H
#include <stdint.h>
#include <stdbool.h>

/* ── 系统调用号枚举（按需扩展） ── */
typedef enum {
    SVC_PEND_SVC      = 1,
    SVC_TASK_SLEEP    ,
    SVC_SEMAPHORE_TAKE     ,
    SVC_SEMAPHORE_GIVE    ,
    SVC_MUTEX_LOCK    ,
    SVC_MUTEX_UNLOCK  ,
    SVC_DEVICE ,
    SVC_SPI_TRANSFER  ,
    SVC_I2C_TRANSFER  ,
    SVC_DELAY_MS      ,
    SVC_GET_TICK      ,
    SVC_TASK_EXIT     ,

    SVC_MPU_PROTECT  ,    // 动态任务栈保护
    SVC_MALLOC       ,
    SVC_FREE         ,
    SVC_MAX_ID
} svc_number_t;

/* ── 系统调用处理函数原型 ── */
typedef uint32_t (*svc_handler_t)(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);

/* ── 系统调用表条目 ── */
typedef struct {
    svc_number_t   number;       /* 系统调用号 */
    svc_handler_t  handler;      /* 处理函数 */
} svc_entry_t;

/* ── 系统调用配置描述符 ── */
typedef struct {
    const svc_entry_t *entries;  /* 系统调用表数组 */
    uint32_t          num_entries; /* 条目数量 */
} svc_config_t;

/* ── SVC 调用宏（0~3 个参数） ── */
#define SVC_CALL0(svc_num) ({              \
    uint32_t _ret;                         \
    __asm volatile ("mov r0, %1\n"         \
                    "svc #0\n"             \
                    "mov %0, r0"           \
                    : "=r" (_ret)          \
                    : "I" (svc_num)        \
                    : "r0", "memory");     \
    _ret; })

#define SVC_CALL1(svc_num, a1) ({          \
    uint32_t _ret;                         \
    __asm volatile ("mov r0, %1\n"         \
                    "mov r1, %2\n"         \
                    "svc #0\n"             \
                    "mov %0, r0"           \
                    : "=r" (_ret)          \
                    : "I" (svc_num), "r" ((uint32_t)(a1)) \
                    : "r0", "r1", "memory"); \
    _ret; })

#define SVC_CALL2(svc_num, a1, a2) ({      \
    uint32_t _ret;                         \
    __asm volatile ("mov r0, %1\n"         \
                    "mov r1, %2\n"         \
                    "mov r2, %3\n"         \
                    "svc #0\n"             \
                    "mov %0, r0"           \
                    : "=r" (_ret)          \
                    : "I" (svc_num), "r" ((uint32_t)(a1)), "r" ((uint32_t)(a2)) \
                    : "r0", "r1", "r2", "memory"); \
    _ret; })

#define SVC_CALL3(svc_num, a1, a2, a3) ({  \
    uint32_t _ret;                         \
    __asm volatile ("mov r0, %1\n"         \
                    "mov r1, %2\n"         \
                    "mov r2, %3\n"         \
                    "mov r3, %4\n"         \
                    "svc #0\n"             \
                    "mov %0, r0"           \
                    : "=r" (_ret)          \
                    : "I" (svc_num), "r" ((uint32_t)(a1)), "r" ((uint32_t)(a2)), "r" ((uint32_t)(a3)) \
                    : "r0", "r1", "r2", "r3", "memory"); \
    _ret; })


/* ========== API ========== */
int  svc_init(const svc_config_t *cfg);
void svc_deinit(void);

/* 内核内部使用的分发函数（由 SVC_Handler 调用） */
uint32_t svc_dispatch(svc_number_t svc_num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);

#endif //STM32F4DISCOVERY_HAL_SVC_H

//
// Created by zhiwei.gong on 2026/5/22.
//

#ifndef STM32F4DISCOVERY_SVC_H
#define STM32F4DISCOVERY_SVC_H
#include "hal/hal_svc.h"


/* ── 用户友好的包装函数 ── */
static inline void start_pendsv_user() {
    SVC_CALL0(SVC_PEND_SVC);
}

static inline char uart_getc_user(void) {
    return (char)SVC_CALL0(SVC_UART_GETC);
}

static inline int uart_write_user(const char *buf, int len) {
    return (int)SVC_CALL2(SVC_UART_WRITE, (uint32_t)buf, (uint32_t)len);
}

static inline void delay_ms_user(uint32_t ms) {
    SVC_CALL1(SVC_DELAY_MS, ms);
}

static inline void task_exit_user(void) {
    SVC_CALL0(SVC_TASK_EXIT);
    while(1);
}

int system_svc_init(void);

#endif //STM32F4DISCOVERY_SVC_H

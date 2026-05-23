//
// Created by zhiwei.gong on 2026/5/22.
//

#include "svc.h"
#include "../common/util.h"
static uint32_t trigger_pendsv(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4);

/* 系统调用表 */
const svc_entry_t svc_entries[] = {
        { SVC_PEND_SVC, trigger_pendsv },
        //{ SVC_UART_WRITE,  my_uart_write },
        //{ SVC_DELAY_MS,    my_delay_ms },
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
}


static uint32_t trigger_pendsv(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    Trigger_PendSV;
}


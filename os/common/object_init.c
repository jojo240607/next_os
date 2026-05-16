#include "object_init.h"
#include "linear_pool.h"
#include "../log/log.h"
#include "sys_time.h"
#include "sys_mutex.h"
#include "../driver/common/pinmux.h"
#include "../driver/common/dma.h"
#include "../driver/common/rcc.h"
#include "../driver/device_config.h"

#include "../scheduler/thread_scheduler.h"
#include "../task/task_manager.h"
#include "../driver/device_manager.h"
#include "../driver/common/nvic.h"

void all_object_init() {
    if (!rcc_sysclk_init(&clk_conf)) {
        // 时钟初始化失败，进入安全模式或重启
        while(1);
    }
    // 现在系统时钟为 168MHz，APB1=42MHz, APB2=84MHz
    // 外设时钟使能示例：开启 GPIOA 时钟 (AHB1 bit0)
    //rcc_periph_clock_enable(RCC_BUS_AHB1, 0);
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);
    NVIC_SetPriority(PendSV_IRQn, 0xff);//NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x03, 0x03));
    NVIC_SetPriority(SysTick_IRQn, 0xff);

    os_pool_init();
    log_init();
    LOG_DEBUG("obj_init", "all_object_init");
    systime_init();
    sys_mutex_init();
    pinmux_init();
    dma_init();


    gloable_nvic = nvic_create();
    global_thread_scheduler = thread_scheduler_create();
    gloable_deviceManager = device_manager_create();
    gloable_taskManager = task_manager_create();
    gloable_taskManager->fun->boot_init(gloable_taskManager);
}

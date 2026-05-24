#include "os_init.h"
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
#include "../driver/hal/hal_flash.h"
#include "../driver/hal/hal_mpu.h"
#include "../driver/svc.h"

/*
 *  enable_fault_trap:
 *    除零陷阱（DIV_0_TRP）
        若开启，当执行整数除法指令（SDIV/UDIV）且除数为 0 时，会立即触发 UsageFault 异常，便于捕获逻辑错误。
        若关闭，除零操作会返回一个不确定的值（通常是 0），但不会报错，可能导致隐蔽的计算错误。
      非对齐访问陷阱（UNALIGN_TRP）
        若开启，当进行非对齐的存储器访问（例如从奇数地址读取 32 位数据）时，会触发 UsageFault。
        若关闭，Cortex-M4 硬件默认支持大部分非对齐访问（但会降低性能），这可能导致偶发的总线错误或数据损坏，特别是在与 DMA 或外设寄存器交互时。
    enable_sleep_on_exit:
        开启后：当处理器完成所有异常处理（例如最后一个中断 ISR 返回）并返回到线程模式时，如果此时没有其他待处理异常，CPU 会立即进入睡眠模式，而不是继续执行主循环代码。
        关闭后：异常返回后总是回到主循环，由软件决定何时睡眠（调用 WFI/WFE）。
    enable_deep_sleep:
        开启后：当执行 WFI/WFE 指令时，处理器会进入深度睡眠模式，此时芯片的功耗管理单元（PWR）会根据配置选择停止模式或待机模式，功耗大幅降低，但唤醒源有限（仅特定外设或引脚）。
        关闭后：WFI/WFE 只会让 CPU 进入普通睡眠模式（仅 CPU 时钟停止，大部分外设仍在运行），唤醒快，功耗稍高。
 *
 * */
static const scb_config_t scb_cfg = {
        .priority_group    = SCB_PRIORITY_GROUP_4,   // 4位抢占优先级
        .vector_table_base = 0x08000000UL,           // 设置中断向量表基地址（VTOR）
        .enable_fault_trap = true,                   // 开启除零/非对齐陷阱
        .enable_sleep_on_exit = false,               //
        .enable_deep_sleep = false,                   // 使能深度睡眠 (配合 WFI)
};
/*
 *
    场景	                            ASPEN	        LSPEN	        说明
    RTOS + FPU	                    ✅ 开启	        ✅ 开启	        硬件自动处理，任务切换无需手动保存 S0-S15
    裸机 + 主循环用浮点 + 中断不用浮点	✅ 开启	        ✅ 开启	        中断不消耗浮点栈，延迟最低
    裸机 + 中断也要用浮点	            ✅ 开启	        ✅ 开启或关闭	    都开则惰性压栈，延迟略低；关掉则入口即压栈，行为更可预测
    完全不用 FPU	                    关闭	             无关	        关闭后硬件不会产生任何浮点压栈，也省电
 *
 * */
static const fpu_config_t fpu_cfg = {
        .mode                     = FPU_MODE_FULL_ACCESS,
        .enable_lazy_stacking     = true,/* 使能惰性堆栈 LSPEN */
        .enable_auto_state_preservation = true, /* 使能自动状态保存 ASPEN */
};
/* 配置 Flash：5 等待周期，开启预取指和缓存 */

/*
 * ART 配置对程序执行速度和 ADC 精度存在影响，具体如下表：
        ART 加速器配置	            性能 (执行速度)	ADC 精度 (噪声)
    (数据+指令) Cache ON，预取 OFF	        高	        最佳 (官方推荐)
    (数据+指令) Cache ON，预取 ON	        最高	        略差
 *
 * */
static const flash_config_t flash_cfg = {
        .latency         = FLASH_LATENCY_5WS,
        .enable_prefetch = false,//预取址
        .enable_icache   = true,//指令缓存
        .enable_dcache   = true,//数据缓存
};

/*
 * SystemInit() 应该做的事
    1.设置 Flash 等待周期（LATENCY）
        这是最重要的一步。如果后续要切换到高频时钟（如 168MHz），但 Flash 的等待周期未提前增加，CPU 取指会出错。因此需要先设置一个安全的 Flash 等待周期（例如 5 WS，对应 150~168 MHz）。后续 rcc_sysclk_init() 中应根据实际频率再次调整。
    2.使能 FPU（浮点单元）
        Cortex-M4F 内核需要手动开启 FPU，否则执行浮点指令会引发 HardFault。你已有 fpu_init()，可以直接调用并传入默认配置（完全访问、自动状态保存）。
    3.设置中断向量表基地址（VTOR）
        如果你的程序在 Flash 起始处运行（0x08000000），VTOR 通常已经正确；但若有 Bootloader 或重定位需求，必须在此处将 SCB->VTOR 指向实际向量表地址。
    4.配置 NVIC 默认优先级分组
        在没有调用 nvic_init() 之前，所有中断都使用默认的分组（通常是 0，即 4 位抢占）。为了在 main() 之前不产生意外中断嵌套，可以提前设置一个常用分组（如 NVIC_PRIORITY_GROUP_4）。
    5.（可选）启用默认时钟源
        系统复位后默认使用 HSI（16 MHz），这已经足够运行到 main()。通常不需要在这里切换时钟，真正的时钟配置留给 main() 中的 rcc_sysclk_init() 完成。
 *
 * */
void SystemInit(void)
{
    hal_flash_init(&flash_cfg);
    hal_fpu_init(&fpu_cfg);   // 内部会设置 CPACR 和 FPCCR
    hal_scb_init(&scb_cfg);
}

void os_init() {
    os_pool_init();
    log_init();
    LOG_DEBUG("os_init", "all_object_init");
    if (!rcc_sysclk_init(&clk_conf)) {
        LOG_ERROR("os_init", "rcc init error");
        // 时钟初始化失败，进入安全模式或重启
        while(1);
    }
    systime_init();
    sys_mutex_init();
    pinmux_init();
    dma_init();
    global_thread_scheduler = thread_scheduler_create();
    gloable_deviceManager = device_manager_create();
    gloable_taskManager = task_manager_create();
    gloable_nvic = nvic_create();
    gloable_taskManager->fun->boot_init(gloable_taskManager);
    system_mpu_setup();
    system_svc_init();
    global_thread_scheduler->fun->start(global_thread_scheduler);
}


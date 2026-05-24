//
// Created by zhiwei.gong on 2026/4/21.
//

#ifndef STM32F4DISCOVERY_UTIL_H
#define STM32F4DISCOVERY_UTIL_H

#include "cmsis_gcc.h"
#include "../driver/hal/hal_nvic.h"

#define OS_STACK_DEBUG (1)
//#define USE_CCMRAM
#define GET_TASK_MANAGER(obj) ((Task_manager *)obj)

//  __DSB() 确保所有内存访问完成
#define USE_CCMRAM

#ifdef USE_CCMRAM
#define CCMRAM __attribute__((section(".ccmram")))
#else
#define CCMRAM  // 在 Renode 模拟器中，宏定义为空，变量将被分配到普通 RAM
#endif

//// 触发 PendSV 中断
#define PENDSVSET ((1UL << 28U))
#define Trigger_PendSV \
    __asm volatile("" ::: "memory"); \
    (xSCB->ICSR |= PENDSVSET); \
    __DSB()

#define LOW_POWER __WFI(); __DSB()

#define ARRAY_SIZE(OBJ) (sizeof(OBJ) / sizeof(OBJ[0]))

#define TO_OBJ(type, obj) ((type *)obj)
#define GET_OBJ_VTAB(type, obj) (*(type##VTable **)obj)

#define DEFAULT_OBJBUF_SIZE (1024 * 10)     //10k 对象池
#define DEFAULT_CCMRAM_SIZE (1024 * 8)     //6k ccm内存池 给任务栈使用
#define DEFAULT_STACK_SIZE (11)              //2 ^ 11 = 2048   默认任务栈大小
#define SYSTEM_TICKS_PER_SEC (1000)         //1000hz / 时间片默认切换周期1ms

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define SYSCALL_IRQ_DEFAULT_PRIO (5 << 4) // IRQ_PREEMPT_PRIORITY_SYSCALL
static inline uint32_t arch_irq_lock(void)
{
    unsigned int key;
    __asm__ volatile (
            "mrs %0, basepri\n\t"          // 1. 读取当前 BASEPRI 值并保存到 'key'
            "movs r0, %1\n\t"
            "msr basepri, r0\n\t"          // 2. 将新的屏蔽值写入 BASEPRI，关中断
            "dsb\n\t"                      // 3. 数据同步屏障
            "isb\n\t"                      // 4. 指令同步屏障，确保立即生效
            : "=r" (key)                   // 输出部分：key 的值为 'r' (通用寄存器)
            : "i" (SYSCALL_IRQ_DEFAULT_PRIO)  // 输入部分：传入立即数作为新屏蔽值
            : "r0", "memory"                     // 可能修改了内存
            );
    return key;
}


static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
    __asm__ volatile (
            "msr basepri, %0\n\t"  // 将传入的 key 写入 BASEPRI，恢复中断
            :                       // 没有输出操作数
            : "r" (key)             // 输入部分：传入通用寄存器中的 key
            : "memory"              // 可能会修改内存
            );
}



#endif //STM32F4DISCOVERY_UTIL_H

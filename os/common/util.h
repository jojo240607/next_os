//
// Created by zhiwei.gong on 2026/4/21.
//

#ifndef STM32F4DISCOVERY_UTIL_H
#define STM32F4DISCOVERY_UTIL_H
#include "main.h"
#define OS_STACK_DEBUG (1)
#define SIMULATION
#define GET_TASK_MANAGER(obj) ((Task_manager *)obj)
//// 触发 PendSV 中断
//  __DSB() 确保所有内存访问完成
#ifdef SIMULATION
#define CCMRAM  // 在 Renode 模拟器中，宏定义为空，变量将被分配到普通 RAM
#else
#define CCMRAM __attribute__((section(".ccmram")))
#endif

#define Trigger_PendSV __asm volatile("" ::: "memory"); (SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk); __DSB()
#define DISABLE_IRQ __disable_irq(); __DSB()
#define ENABLE_IRQ __enable_irq(); __DSB()
#define LOW_POWER __WFI(); __DSB()

#define ARRAY_SIZE(OBJ) (sizeof(OBJ) / sizeof(OBJ[0]))

#define TO_OBJ(type, obj) ((type *)obj)
#define GET_OBJ_VTAB(type, obj) (*(type##VTable **)obj)

#define DEFAULT_OBJBUF_SIZE (1024 * 10)     //10k 对象池
#define DEFAULT_CCMRAM_SIZE (1024 * 6)     //6k ccm内存池 给任务栈使用
#define DEFAULT_STACK_SIZE (512)            //512 x 4 = 2k   默认任务栈大小
#define SYSTEM_TICKS_PER_SEC (1000)         //1000hz / 时间片默认切换周期1ms

#endif //STM32F4DISCOVERY_UTIL_H

//
// Created by zhiwei.gong on 2026/4/21.
//

#ifndef STM32F4DISCOVERY_UTIL_H
#define STM32F4DISCOVERY_UTIL_H
#include "main.h"

#define GET_TASK_MANAGER(obj) ((Task_manager *)obj)
//// 触发 PendSV 中断
#define Trigger_PendSV (SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk)
#define DISABLE_IRQ __disable_irq()
#define ENABLE_IRQ __enable_irq()
#define LOW_POWER __WFI()
#define ARRAY_SIZE(OBJ) (sizeof(OBJ) / sizeof(OBJ[0]))

#endif //STM32F4DISCOVERY_UTIL_H

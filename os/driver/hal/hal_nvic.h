//
// Created by zhiwei.gong on 2026/5/19.
//

#ifndef STM32F4DISCOVERY_HAL_NVIC_H
#define STM32F4DISCOVERY_HAL_NVIC_H
#include <stdint.h>
#include <stdbool.h>
#include "hal_scb.h"


/* 中断号类型（由 CMSIS 头文件提供） */
typedef enum : int8_t {
    /******  Cortex-M4 Processor Exceptions Numbers ****************************************************************/
    xNonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                          */
    xMemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M4 Memory Management Interrupt                           */
    xBusFault_IRQn               = -11,    /*!< 5 Cortex-M4 Bus Fault Interrupt                                   */
    xUsageFault_IRQn             = -10,    /*!< 6 Cortex-M4 Usage Fault Interrupt                                 */
    xSVCall_IRQn                 = -5,     /*!< 11 Cortex-M4 SV Call Interrupt                                    */
    xDebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M4 Debug Monitor Interrupt                              */
    xPendSV_IRQn                 = -2,     /*!< 14 Cortex-M4 Pend SV Interrupt                                    */
    xSysTick_IRQn                = -1,     /*!< 15 Cortex-M4 System Tick Interrupt                                */
    /******  STM32 specific Interrupt Numbers **********************************************************************/
    xWWDG_IRQn                   = 0,      /*!< Window WatchDog Interrupt                                         */
    xPVD_IRQn                    = 1,      /*!< PVD through EXTI Line detection Interrupt                         */
    xTAMP_STAMP_IRQn             = 2,      /*!< Tamper and TimeStamp interrupts through the EXTI line             */
    xRTC_WKUP_IRQn               = 3,      /*!< RTC Wakeup interrupt through the EXTI line                        */
    xFLASH_IRQn                  = 4,      /*!< FLASH global Interrupt                                            */
    xRCC_IRQn                    = 5,      /*!< RCC global Interrupt                                              */
    xEXTI0_IRQn                  = 6,      /*!< EXTI Line0 Interrupt                                              */
    xEXTI1_IRQn                  = 7,      /*!< EXTI Line1 Interrupt                                              */
    xEXTI2_IRQn                  = 8,      /*!< EXTI Line2 Interrupt                                              */
    xEXTI3_IRQn                  = 9,      /*!< EXTI Line3 Interrupt                                              */
    xEXTI4_IRQn                  = 10,     /*!< EXTI Line4 Interrupt                                              */
    xDMA1_Stream0_IRQn           = 11,     /*!< DMA1 Stream 0 global Interrupt                                    */
    xDMA1_Stream1_IRQn           = 12,     /*!< DMA1 Stream 1 global Interrupt                                    */
    xDMA1_Stream2_IRQn           = 13,     /*!< DMA1 Stream 2 global Interrupt                                    */
    xDMA1_Stream3_IRQn           = 14,     /*!< DMA1 Stream 3 global Interrupt                                    */
    xDMA1_Stream4_IRQn           = 15,     /*!< DMA1 Stream 4 global Interrupt                                    */
    xDMA1_Stream5_IRQn           = 16,     /*!< DMA1 Stream 5 global Interrupt                                    */
    xDMA1_Stream6_IRQn           = 17,     /*!< DMA1 Stream 6 global Interrupt                                    */
    xADC_IRQn                    = 18,     /*!< ADC1, ADC2 and ADC3 global Interrupts                             */
    xCAN1_TX_IRQn                = 19,     /*!< CAN1 TX Interrupt                                                 */
    xCAN1_RX0_IRQn               = 20,     /*!< CAN1 RX0 Interrupt                                                */
    xCAN1_RX1_IRQn               = 21,     /*!< CAN1 RX1 Interrupt                                                */
    xCAN1_SCE_IRQn               = 22,     /*!< CAN1 SCE Interrupt                                                */
    xEXTI9_5_IRQn                = 23,     /*!< External Line[9:5] Interrupts                                     */
    xTIM1_BRK_TIM9_IRQn          = 24,     /*!< TIM1 Break interrupt and TIM9 global interrupt                    */
    xTIM1_UP_TIM10_IRQn          = 25,     /*!< TIM1 Update Interrupt and TIM10 global interrupt                  */
    xTIM1_TRG_COM_TIM11_IRQn     = 26,     /*!< TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt */
    xTIM1_CC_IRQn                = 27,     /*!< TIM1 Capture Compare Interrupt                                    */
    xTIM2_IRQn                   = 28,     /*!< TIM2 global Interrupt                                             */
    xTIM3_IRQn                   = 29,     /*!< TIM3 global Interrupt                                             */
    xTIM4_IRQn                   = 30,     /*!< TIM4 global Interrupt                                             */
    xI2C1_EV_IRQn                = 31,     /*!< I2C1 Event Interrupt                                              */
    xI2C1_ER_IRQn                = 32,     /*!< I2C1 Error Interrupt                                              */
    xI2C2_EV_IRQn                = 33,     /*!< I2C2 Event Interrupt                                              */
    xI2C2_ER_IRQn                = 34,     /*!< I2C2 Error Interrupt                                              */
    xSPI1_IRQn                   = 35,     /*!< SPI1 global Interrupt                                             */
    xSPI2_IRQn                   = 36,     /*!< SPI2 global Interrupt                                             */
    xUSART1_IRQn                 = 37,     /*!< USART1 global Interrupt                                           */
    xUSART2_IRQn                 = 38,     /*!< USART2 global Interrupt                                           */
    xUSART3_IRQn                 = 39,     /*!< USART3 global Interrupt                                           */
    xEXTI15_10_IRQn              = 40,     /*!< External Line[15:10] Interrupts                                   */
    xRTC_Alarm_IRQn              = 41,     /*!< RTC Alarm (A and B) through EXTI Line Interrupt                   */
    xOTG_FS_WKUP_IRQn            = 42,     /*!< USB OTG FS Wakeup through EXTI line interrupt                     */
    xTIM8_BRK_TIM12_IRQn         = 43,     /*!< TIM8 Break Interrupt and TIM12 global interrupt                   */
    xTIM8_UP_TIM13_IRQn          = 44,     /*!< TIM8 Update Interrupt and TIM13 global interrupt                  */
    xTIM8_TRG_COM_TIM14_IRQn     = 45,     /*!< TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt */
    xTIM8_CC_IRQn                = 46,     /*!< TIM8 Capture Compare global interrupt                             */
    xDMA1_Stream7_IRQn           = 47,     /*!< DMA1 Stream7 Interrupt                                            */
    xFSMC_IRQn                   = 48,     /*!< FSMC global Interrupt                                             */
    xSDIO_IRQn                   = 49,     /*!< SDIO global Interrupt                                             */
    xTIM5_IRQn                   = 50,     /*!< TIM5 global Interrupt                                             */
    xSPI3_IRQn                   = 51,     /*!< SPI3 global Interrupt                                             */
    xUART4_IRQn                  = 52,     /*!< UART4 global Interrupt                                            */
    xUART5_IRQn                  = 53,     /*!< UART5 global Interrupt                                            */
    xTIM6_DAC_IRQn               = 54,     /*!< TIM6 global and DAC1&2 underrun error  interrupts                 */
    xTIM7_IRQn                   = 55,     /*!< TIM7 global interrupt                                             */
    xDMA2_Stream0_IRQn           = 56,     /*!< DMA2 Stream 0 global Interrupt                                    */
    xDMA2_Stream1_IRQn           = 57,     /*!< DMA2 Stream 1 global Interrupt                                    */
    xDMA2_Stream2_IRQn           = 58,     /*!< DMA2 Stream 2 global Interrupt                                    */
    xDMA2_Stream3_IRQn           = 59,     /*!< DMA2 Stream 3 global Interrupt                                    */
    xDMA2_Stream4_IRQn           = 60,     /*!< DMA2 Stream 4 global Interrupt                                    */
    xETH_IRQn                    = 61,     /*!< Ethernet global Interrupt                                         */
    xETH_WKUP_IRQn               = 62,     /*!< Ethernet Wakeup through EXTI line Interrupt                       */
    xCAN2_TX_IRQn                = 63,     /*!< CAN2 TX Interrupt                                                 */
    xCAN2_RX0_IRQn               = 64,     /*!< CAN2 RX0 Interrupt                                                */
    xCAN2_RX1_IRQn               = 65,     /*!< CAN2 RX1 Interrupt                                                */
    xCAN2_SCE_IRQn               = 66,     /*!< CAN2 SCE Interrupt                                                */
    xOTG_FS_IRQn                 = 67,     /*!< USB OTG FS global Interrupt                                       */
    xDMA2_Stream5_IRQn           = 68,     /*!< DMA2 Stream 5 global interrupt                                    */
    xDMA2_Stream6_IRQn           = 69,     /*!< DMA2 Stream 6 global interrupt                                    */
    xDMA2_Stream7_IRQn           = 70,     /*!< DMA2 Stream 7 global interrupt                                    */
    xUSART6_IRQn                 = 71,     /*!< USART6 global interrupt                                           */
    xI2C3_EV_IRQn                = 72,     /*!< I2C3 event interrupt                                              */
    xI2C3_ER_IRQn                = 73,     /*!< I2C3 error interrupt                                              */
    xOTG_HS_EP1_OUT_IRQn         = 74,     /*!< USB OTG HS End Point 1 Out global interrupt                       */
    xOTG_HS_EP1_IN_IRQn          = 75,     /*!< USB OTG HS End Point 1 In global interrupt                        */
    xOTG_HS_WKUP_IRQn            = 76,     /*!< USB OTG HS Wakeup through EXTI interrupt                          */
    xOTG_HS_IRQn                 = 77,     /*!< USB OTG HS global interrupt                                       */
    xDCMI_IRQn                   = 78,     /*!< DCMI global interrupt                                             */
    xRNG_IRQn                    = 80,     /*!< RNG global Interrupt                                              */
    xFPU_IRQn                    = 81      /*!< FPU global interrupt                                               */
} nvic_irqn_t;

/*
 *
 *
    中断	        优先级 (逻辑优先级顺序)	是否被屏蔽	原因
    紧急刹车信号	1 (最高)	            否	        1 < 5，优先级高于门槛，能正常响应。
    电机控制	    3 (高)	            否	        3 < 5，优先级高于门槛，能正常响应。
    按键输入	    5 (中)	            是	        5 >= 5，优先级低于或等于门槛，被屏蔽。
    串口接收	    7 (中低)          	是	        7 >= 5，优先级低于门槛，被屏蔽。
    空闲任务	    15 (最低)	        是	        被完全屏蔽。

 */
typedef enum : uint8_t {
    IRQ_PREEMPT_PRIORITY_HIGHEST = 0,//最高优先级
    IRQ_PREEMPT_PRIORITY_VERY_HIGH3,
    IRQ_PREEMPT_PRIORITY_VERY_HIGH2,
    IRQ_PREEMPT_PRIORITY_VERY_HIGH1,
    IRQ_PREEMPT_PRIORITY_VERY_HIGH0,
    IRQ_PREEMPT_PRIORITY_SYSCALL, // 系统调度级别,高于此的可打断任务切换中断
    IRQ_PREEMPT_PRIORITY_HIGH2,
    IRQ_PREEMPT_PRIORITY_HIGH1,
    IRQ_PREEMPT_PRIORITY_HIGH0,
    IRQ_PREEMPT_PRIORITY_MIDDLE1,
    IRQ_PREEMPT_PRIORITY_MIDDLE0,
    IRQ_PREEMPT_PRIORITY_NORMAL,   //中等级别中断优先级
    IRQ_PREEMPT_PRIORITY_LOW0,
    IRQ_PREEMPT_PRIORITY_LOW1,
    IRQ_PREEMPT_PRIORITY_LOW2,
    IRQ_PREEMPT_PRIORITY_LOWEST,//最低优先级
} nvic_priority_t;

/* 单条中断配置描述符 */
typedef struct {
    nvic_irqn_t     irq;                // 中断号 (IRQn_Type)
    nvic_priority_t     preempt_priority;   // 抢占优先级 (0..15)
    uint8_t     sub_priority;       // 子优先级 (0..15)
    bool        enable;             // 配置完成后是否使能
} nvic_irq_config_t;

/* NVIC 总配置描述符 */
typedef struct {
    scb_priority_group_t   priority_group;     // 全局优先级分组
    uint32_t                num_irqs;           // 配置的中断数量
    const nvic_irq_config_t *irq_configs[];       // 中断配置数组指针
} nvic_config_t;

/* ========== API ========== */
int  hal_nvic_init(const nvic_config_t *cfg);
void hal_nvic_deinit(void);

void hal_nvic_set_priority_group(scb_priority_group_t group);
void hal_nvic_enable_irq(nvic_irqn_t irq);
void hal_nvic_disable_irq(nvic_irqn_t irq);
void hal_nvic_set_priority(nvic_irqn_t irq, nvic_priority_t preempt_priority, uint8_t sub_priority);
void hal_nvic_set_pending(nvic_irqn_t irq);
void hal_nvic_clear_pending(nvic_irqn_t irq);
uint32_t hal_nvic_get_active(nvic_irqn_t irq);
bool hal_nvic_is_enabled(nvic_irqn_t irq);

/* 全局中断控制 */
void hal_nvic_global_irq_enable(void);
void hal_nvic_global_irq_disable(void);

/* 系统复位 */
void hal_nvic_system_reset(void);
#endif //STM32F4DISCOVERY_HAL_NVIC_H

#ifndef NVIC_H
#define NVIC_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../scheduler/semaphore.h"
#include "../hal/hal_nvic.h"
#include "../../task/task.h"

#define GET_Nvic(obj) ((Nvic *)obj)
// 类声明
typedef struct _Nvic Nvic;
typedef struct _NvicFun NvicFun;
typedef struct _nvic_irq_t nvic_irq_t;
// 中断上半部回调函数原型（运行于中断上下文）
typedef bool (*nvic_handler_t)(nvic_irq_t *irq_conf);
typedef enum : uint8_t {
    SYSTIC_IRQ = 0,
    USART1_IRQ,
    USART2_IRQ,
    USART3_IRQ,
    USART4_IRQ,
    USART5_IRQ,
    USART6_IRQ,
    EXTI0_IRQ,
    EXTI1_IRQ,
    EXTI2_IRQ,
    EXTI3_IRQ,
    EXTI4_IRQ,
    EXTI5_9_IRQ,
    EXTI10_15_IRQ,
    DMA1_ST0_IRQ,
    DMA1_ST1_IRQ,
    DMA1_ST2_IRQ,
    DMA1_ST3_IRQ,
    DMA1_ST4_IRQ,
    DMA1_ST5_IRQ,
    DMA1_ST6_IRQ,
    DMA1_ST7_IRQ,
    DMA2_ST0_IRQ,
    DMA2_ST1_IRQ,
    DMA2_ST2_IRQ,
    DMA2_ST3_IRQ,
    DMA2_ST4_IRQ,
    DMA2_ST5_IRQ,
    DMA2_ST6_IRQ,
    DMA2_ST7_IRQ,
    ADC_IRQ,
    CAN1_TX_IRQ,
    CAN1_RX0_IRQ,
    CAN1_RX1_IRQ,
    CAN1_SCE_IRQ,
    CAN2_TX_IRQ,
    CAN2_RX0_IRQ,
    CAN2_RX1_IRQ,
    CAN2_SCE_IRQ,
    I2C1_EV_IRQ,
    I2C1_ER_IRQ,
    I2C2_EV_IRQ,
    I2C2_ER_IRQ,
    I2C3_EV_IRQ,
    I2C3_ER_IRQ,
    SPI1_IRQ,
    SPI2_IRQ,
    SPI3_IRQ,
    SDIO_IRQ,
    //FSMC_IRQ,
    TIM1_BRK_TIM9_IRQ,
    TIM1_UP_TIM10_IRQ,
    TIM1_TRG_COM_TIM11_IRQ,
    TIM1_CC_IRQ,
    TIM2_IRQ,
    TIM3_IRQ,
    TIM4_IRQ,
    TIM5_IRQ,
    TIM6_DAC_IRQ,
    TIM7_IRQ,
    TIM8_BRK_TIM12_IRQ,
    TIM8_UP_TIM13_IRQ,
    TIM8_TRG_COM_TIM14_IRQ,
    TIM8_CC_IRQ,
    RTC_ALARM_IRQ,
    RTC_WKUP_IRQ,
    OTG_FS_IRQ,
    UsageFault_IRQ,
    MAX_IRQ
} nvic_irq_num;

// 类成员函数结构（当前使用全局函数 + gloable_nvic 单例模式）
struct _NvicFun {
    void (*destroy)(Nvic* self);
};



// 中断控制块
struct _nvic_irq_t {
   // Node base;
    nvic_irq_num id;
    nvic_handler_t handler;      // 上半部回调
    void *arg;                   // 回调参数
    Task *bottom_task;          // 关联的下半部任务函数.（可为 NULL）
    void *event;                // 事件指针 该事件在中断后可以传递到中断任务函数中
    bool registered;            // 是否已注册
};

// 类结构
struct _Nvic {
    const NvicFun* fun;
    // TODO: 添加数据成员
    nvic_irq_t *irq_table[];
};

// 构造函数声明
Nvic* nvic_create();
void nvic_init(Nvic* self);

// 析构函数声明
void nvic_deinit(Nvic* self);
// 全局中断分发函数，在具体的中断处理函数中调用
void dispatch(nvic_irq_num irq_num);
extern Nvic *gloable_nvic;
bool nvic_register(Nvic* self, nvic_irq_num irq_num, nvic_handler_t handler, void *arg, void *event);
void nvic_unregister(Nvic* self, nvic_irq_num irq_num);
void nvic_attach_task(Nvic* self, nvic_irq_num irq_num, Task *task);
void nvic_set_priority(Nvic* self, nvic_irq_num irq_num, nvic_priority_t preempt_priority, uint8_t sub_priority);
void nvic_dispatch(Nvic* self, nvic_irq_num irq_num);

void NMI_Handler(void);
void HardFault_Handler(void);

void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);

void SysTick_Handler(void);
void USART1_IRQHandler();
void USART3_IRQHandler();
void UART4_IRQHandler();
void TIM2_IRQHandler();

void EXTI0_IRQHandler();
void EXTI1_IRQHandler();
void EXTI9_5_IRQHandler();
void EXTI15_10_IRQHandler();
void I2C1_EV_IRQHandler();
void I2C1_ER_IRQHandler();
void SPI1_IRQHandler();

void DMA1_Stream0_IRQHandler();
void DMA1_Stream1_IRQHandler();
void DMA1_Stream2_IRQHandler();
void DMA1_Stream3_IRQHandler();
void DMA1_Stream4_IRQHandler();
void DMA1_Stream5_IRQHandler();
void DMA1_Stream6_IRQHandler();
void DMA1_Stream7_IRQHandler();

void DMA2_Stream0_IRQHandler();
void DMA2_Stream1_IRQHandler();
void DMA2_Stream2_IRQHandler();
void DMA2_Stream3_IRQHandler();
void DMA2_Stream4_IRQHandler();
void DMA2_Stream5_IRQHandler();
void DMA2_Stream6_IRQHandler();
void DMA2_Stream7_IRQHandler();

void OTG_FS_IRQHandler(void);
#endif // NVIC_H
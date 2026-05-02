#ifndef INTC_H
#define INTC_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../scheduler/semaphore.h"

#define INTC_MAX_IRQ 82
#define GET_INTC(obj) ((Intc *)obj)
// 类声明
typedef struct _Intc Intc;
typedef struct _IntcFun IntcFun;
typedef struct _intc_irq_t intc_irq_t;
typedef enum _Intc_Irq intc_irq_num;
// 中断上半部回调函数原型（运行于中断上下文）
typedef bool (*intc_handler_t)(void *arg);
enum _Intc_Irq {
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
    //DMA1_CH0_IRQ,
    //DMA1_CH1_IRQ,
    //DMA1_CH2_IRQ,
    //DMA1_CH3_IRQ,
    //DMA1_CH4_IRQ,
    //DMA1_CH5_IRQ,
    //DMA1_CH6_IRQ,
    //DMA1_CH7_IRQ,
    //DMA2_CH0_IRQ,
    //DMA2_CH1_IRQ,
    //DMA2_CH2_IRQ,
    //DMA2_CH3_IRQ,
    //DMA2_CH4_IRQ,
    //DMA2_CH5_IRQ,
    //DMA2_CH6_IRQ,
    //DMA2_CH7_IRQ,
    //ADC_IRQ,
    //CAN1_TX_IRQ,
    //CAN1_RX0_IRQ,
    //CAN1_RX1_IRQ,
    //CAN1_SCE_IRQ,
    //CAN2_TX_IRQ,
    //CAN2_RX0_IRQ,
    //CAN2_RX1_IRQ,
    //CAN2_SCE_IRQ,
    //I2C1_EV_IRQ,
    //I2C1_ER_IRQ,
    //I2C2_EV_IRQ,
    //I2C2_ER_IRQ,
    //I2C3_EV_IRQ,
    //I2C3_ER_IRQ,
    //SPI1_IRQ,
    //SPI2_IRQ,
    //SPI3_IRQ,
    //SDIO_IRQ,
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
    MAX_IRQ
};

// 类成员函数结构
struct _IntcFun {
    void (*destroy)(Intc* self);
    bool (*register_handler)(Intc* self, intc_irq_num irq_num, intc_handler_t handler, void *arg);
	void (*unregister_handler)(Intc* self, intc_irq_num irq_num);
	void (*attach_semaphore)(Intc* self, intc_irq_num irq_num, Semaphore *sem);
	void (*set_priority)(Intc* self, intc_irq_num irq_num, uint32_t priority);
};
// 中断控制块
struct _intc_irq_t {
    Node base;
    intc_handler_t handler;      // 上半部回调
    void *arg;                   // 回调参数
    Semaphore *bottom_sem;     // 关联的下半部信号量（可为 NULL）
    bool registered;          // 是否已注册
};

// 类结构
struct _Intc {
    const IntcFun* fun;
    // TODO: 添加数据成员
    //intc_irq_t *irq_tab;
    intc_irq_t irq_table[];
};

// 构造函数声明
Intc* intc_create();
void intc_init(Intc* self);

// 析构函数声明
void intc_deinit(Intc* self);
// 全局中断分发函数，在具体的中断处理函数中调用
void dispatch(intc_irq_num irq_num);
extern Intc *gloable_intc;
#endif // INTC_H
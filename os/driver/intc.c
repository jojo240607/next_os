#include "intc.h"
#include <stdio.h>
#include "../common/util.h"
#include "../common/linear_pool.h"
#include "../log/log.h"

static bool intc_register(Intc* self, intc_irq_num irq_num, intc_handler_t handler, void *arg);
static void intc_unregister(Intc* self, intc_irq_num irq_num);
static void intc_attach_semaphore(Intc* self, intc_irq_num irq_num, Semaphore *sem);
static void intc_set_priority(Intc* self, intc_irq_num irq_num, uint32_t priority);
static void intc_dispatch(Intc* self, intc_irq_num irq_num);

// 析构函数声明
static void intc_destroy(Intc* self);
Intc *gloable_intc = NULL;
// TODO: 初始化数据成员
static const IntcFun intc_fun = {
    .destroy = intc_destroy,
	.register_handler = intc_register,
	.unregister_handler = intc_unregister,
	.attach_semaphore = intc_attach_semaphore,
	.set_priority = intc_set_priority,
};
static const IRQn_Type irq_num_to_vt[] = {
    SysTick_IRQn,
    USART1_IRQn,
    USART2_IRQn,
    USART3_IRQn,
    UART4_IRQn,
    UART5_IRQn,
    USART6_IRQn,
    EXTI0_IRQn,
    EXTI1_IRQn,
    EXTI2_IRQn,
    EXTI3_IRQn,
    EXTI4_IRQn,
    EXTI9_5_IRQn,
    EXTI15_10_IRQn,
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
        //TIM1_BRK_TIM9_IRQ,
        //TIM1_UP_TIM10_IRQ,
        //TIM1_TRG_COM_TIM11_IRQ,
        //TIM1_CC_IRQ,
    TIM2_IRQn,
        //TIM3_IRQ,
        //TIM4_IRQ,
        //TIM5_IRQ,
        //TIM6_DAC_IRQ,
        //TIM7_IRQ,
        //TIM8_BRK_TIM12_IRQ,
        //TIM8_UP_TIM13_IRQ,
        //TIM8_TRG_COM_TIM14_IRQ,
        //TIM8_CC_IRQ,
        //RTC_ALARM_IRQ,
};
// 构造函数实现
Intc* intc_create() {
    Intc* obj = (Intc*)os_malloc(sizeof(Intc) + sizeof(intc_irq_t) * MAX_IRQ);
    if (obj) {
        memset(obj, 0, sizeof(Intc) + sizeof(intc_irq_t) * MAX_IRQ);
        intc_init(obj);
    }
    return obj;
}

void intc_init(Intc* self) {
    LOG_DEBUG("intc","intc_init");
    self->fun = &(intc_fun);
    // TODO: 初始化数据成员
}

void intc_deinit(Intc* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void intc_destroy(Intc* self) {
    if (self != NULL) {
        intc_deinit(self);
        os_free(self);
    }
}


// register method
static bool intc_register(Intc* self, intc_irq_num irq_num, intc_handler_t handler, void *arg) {
    if (self == NULL) {
        return false;
    }
    if (irq_num >= MAX_IRQ || handler == NULL) {
        return false;
    }
    if ((self->irq_table + irq_num)->registered) {
        return false;
    }
    // 临界区保护（关中断）
    DISABLE_IRQ;
    if ((self->irq_table + irq_num)->handler == NULL) {
        (self->irq_table + irq_num)->handler = handler;
    }
    (self->irq_table + irq_num)->arg = arg;
    (self->irq_table + irq_num)->registered = true;
    // 使能 NVIC 对应中断（假设已设置优先级）
    NVIC_EnableIRQ((IRQn_Type)irq_num);
    ENABLE_IRQ;
    return true;
}
// unregister method
static void intc_unregister(Intc* self, intc_irq_num irq_num) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ) {
        return;
    }
    DISABLE_IRQ;
    (self->irq_table + irq_num)->handler = NULL;
    (self->irq_table + irq_num)->arg = NULL;
    (self->irq_table + irq_num)->bottom_sem = NULL;
    (self->irq_table + irq_num)->registered = false;
    // 可选：禁用 NVIC 中断
    NVIC_DisableIRQ((IRQn_Type)irq_num);
    ENABLE_IRQ;
}
// attach_semaphore method
static void intc_attach_semaphore(Intc* self, intc_irq_num irq_num, Semaphore *sem) {
    if (irq_num >= MAX_IRQ || sem == NULL)
        return;
    DISABLE_IRQ;
    (self->irq_table + irq_num)->bottom_sem = sem;
    ENABLE_IRQ;
}
// set_priority method
static void intc_set_priority(Intc* self, intc_irq_num irq_num, uint32_t priority) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ) {
        return;
    }

    NVIC_SetPriority(irq_num_to_vt[irq_num], priority);
    NVIC_EnableIRQ(irq_num_to_vt[irq_num]);

}
// dispatch method
static void intc_dispatch(Intc* self, intc_irq_num irq_num) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ)
        return;

    intc_irq_t *irq = self->irq_table + irq_num;
    if (irq == NULL) {
        return;
    }
    // 1. 执行上半部回调
    while (irq != NULL) {
        if (irq->handler) {
            if (irq->handler(irq->arg)) {
                // 2. 如果关联了下半部信号量，释放它（注意：此函数在中断中，应使用 from_isr 版本）
                if (irq->bottom_sem) {
                    // 假设你的信号量有 semaphore_give_from_isr 函数
                    // 并根据返回值决定是否需要请求调度
                    irq->bottom_sem->fun->give(irq->bottom_sem);
                }
            }
            break;
        }
        irq = (intc_irq_t *)GET_NODE(irq)->next;
    }

}

void dispatch(intc_irq_num irq_num) {
    intc_dispatch(gloable_intc, irq_num);
}


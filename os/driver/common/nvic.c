#include "nvic.h"
#include <stdio.h>
#include "../../common/util.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../hal/hal_nvic.h"
#include "../hal/hal_fault_diag.h"



/* NVIC 初始化：设置优先级分组 + 批量配置中断 */
static const nvic_config_t nvic_cfg = {
        .priority_group = SCB_PRIORITY_GROUP_4,  // 4位抢占，0位子优先级
        .num_irqs       = 2,
        .irq_configs    = {&(const nvic_irq_config_t) {
                            .irq = xPendSV_IRQn,
                            .preempt_priority = IRQ_PREEMPT_PRIORITY_LOWEST,
                            .sub_priority = 0,      //0011
                            .enable = false},
                           &(const nvic_irq_config_t) {
                                   .irq = xSVCall_IRQn,
                                   .preempt_priority = IRQ_PREEMPT_PRIORITY_SYSCALL,
                                   .sub_priority = 0,
                                   .enable = false}
        }
};

// 析构函数声明
static void nvic_destroy(Nvic* self);
Nvic *gloable_nvic = NULL;
// TODO: 初始化数据成员
static const NvicFun nvic_fun = {
    .destroy = nvic_destroy,
	//.register_handler = nvic_register,
	//.unregister_handler = nvic_unregister,
	//.attach_semaphore = nvic_attach_semaphore,
	//.set_priority = nvic_set_priority,
};

typedef struct {
    nvic_irqn_t irqn;
    const char *irq_name;
} nvic_irq_type_t;

static const nvic_irq_type_t IRQx[] = {
        {xSysTick_IRQn, "xSysTick_IRQn"},
        {xUSART1_IRQn, "xUSART1_IRQn"},
        {xUSART2_IRQn, "xUSART2_IRQn"},
        {xUSART3_IRQn, "xUSART3_IRQn"},
        {xUART4_IRQn, "xUART4_IRQn"},
        {xUART5_IRQn, "xUART5_IRQn"},
        {xUSART6_IRQn, "xUSART6_IRQn"},
        {xEXTI0_IRQn, "xEXTI0_IRQn"},
        {xEXTI1_IRQn, "xEXTI1_IRQn"},
        {xEXTI2_IRQn, "xEXTI2_IRQn"},
        {xEXTI3_IRQn, "xEXTI3_IRQn"},
        {xEXTI4_IRQn, "xEXTI4_IRQn"},
        {xEXTI9_5_IRQn, "xEXTI9_5_IRQn"},
        {xEXTI15_10_IRQn, "xEXTI15_10_IRQn"},
        {xDMA1_Stream0_IRQn, "xDMA1_Stream0_IRQn"},
        {xDMA1_Stream1_IRQn, "xDMA1_Stream1_IRQn"},
        {xDMA1_Stream2_IRQn, "xDMA1_Stream2_IRQn"},
        {xDMA1_Stream3_IRQn, "xDMA1_Stream3_IRQn"},
        {xDMA1_Stream4_IRQn, "xDMA1_Stream4_IRQn"},
        {xDMA1_Stream5_IRQn, "xDMA1_Stream5_IRQn"},
        {xDMA1_Stream6_IRQn, "xDMA1_Stream6_IRQn"},
        {xDMA1_Stream7_IRQn, "xDMA1_Stream7_IRQn"},
        {xDMA2_Stream0_IRQn, "xDMA2_Stream0_IRQn"},
        {xDMA2_Stream1_IRQn, "xDMA2_Stream1_IRQn"},
        {xDMA2_Stream2_IRQn, "xDMA2_Stream2_IRQn"},
        {xDMA2_Stream3_IRQn, "xDMA2_Stream3_IRQn"},
        {xDMA2_Stream4_IRQn, "xDMA2_Stream4_IRQn"},
        {xDMA2_Stream5_IRQn, "xDMA2_Stream5_IRQn"},
        {xDMA2_Stream6_IRQn, "xDMA2_Stream6_IRQn"},
        {xDMA2_Stream7_IRQn, "xDMA2_Stream7_IRQn"},
        {xADC_IRQn, "xADC_IRQn"},
        {xCAN1_TX_IRQn, "xCAN1_TX_IRQn"},
        {xCAN1_RX0_IRQn, "xCAN1_RX0_IRQn"},
        {xCAN1_RX1_IRQn, "xCAN1_RX1_IRQn"},
        {xCAN1_SCE_IRQn, "xCAN1_SCE_IRQn"},
        {xCAN2_TX_IRQn, "xCAN2_TX_IRQn"},
        {xCAN2_RX0_IRQn, "xCAN2_RX0_IRQn"},
        {xCAN2_RX1_IRQn, "xCAN2_RX1_IRQn"},
        {xCAN2_SCE_IRQn, "xCAN2_SCE_IRQn"},
        {xI2C1_EV_IRQn, "xI2C1_EV_IRQn"},
        {xI2C1_ER_IRQn, "xI2C1_ER_IRQn"},
        {xI2C2_EV_IRQn, "xI2C2_EV_IRQn"},
        {xI2C2_ER_IRQn, "xI2C2_ER_IRQn"},
        {xI2C3_EV_IRQn, "xI2C3_EV_IRQn"},
        {xI2C3_ER_IRQn, "xI2C3_ER_IRQn"},
        {xSPI1_IRQn, "xSPI1_IRQn"},
        {xSPI2_IRQn, "xSPI2_IRQn"},
        {xSPI3_IRQn, "xSPI3_IRQn"},
        {xSDIO_IRQn, "xSDIO_IRQn"},
        //{FSMC_IRQ, "FSMC_IRQ"},
        {xTIM1_BRK_TIM9_IRQn, "xTIM1_BRK_TIM9_IRQn"},
        {xTIM1_UP_TIM10_IRQn, "xTIM1_UP_TIM10_IRQn"},
        {xTIM1_TRG_COM_TIM11_IRQn, "xTIM1_TRG_COM_TIM11_IRQn"},
        {xTIM1_CC_IRQn, "xTIM1_CC_IRQn"},
        {xTIM2_IRQn, "xTIM2_IRQn"},
        {xTIM3_IRQn, "xTIM3_IRQn"},
        {xTIM4_IRQn, "xTIM4_IRQn"},
        {xTIM5_IRQn, "xTIM5_IRQn"},
        {xTIM6_DAC_IRQn, "xTIM6_DAC_IRQn"},
        {xTIM7_IRQn, "xTIM7_IRQn"},
        {xTIM8_BRK_TIM12_IRQn, "xTIM8_BRK_TIM12_IRQn"},
        {xTIM8_UP_TIM13_IRQn, "xTIM8_UP_TIM13_IRQn"},
        {xTIM8_TRG_COM_TIM14_IRQn, "xTIM8_TRG_COM_TIM14_IRQn"},
        {xTIM8_CC_IRQn, "xTIM8_CC_IRQn"},
        {xRTC_Alarm_IRQn, "xRTC_Alarm_IRQn"},
        {xRTC_WKUP_IRQn, "xRTC_WKUP_IRQn"},
        {xOTG_FS_IRQn, "xOTG_FS_IRQn"},
        {xUsageFault_IRQn, "xUsageFault_IRQn"},
};
// 构造函数实现
Nvic* nvic_create() {
    Nvic* obj = (Nvic*)os_malloc(sizeof(Nvic) + sizeof(nvic_irq_t) * MAX_IRQ);
    if (obj) {
        memset(obj, 0, sizeof(Nvic) + sizeof(nvic_irq_t) * MAX_IRQ);
        nvic_init(obj);
    }
    return obj;
}

void nvic_init(Nvic* self) {
    LOG_DEBUG("nvic","nvic_init");
    self->fun = &(nvic_fun);
    // TODO: 初始化数据成员
    LOG_DEBUG("nvic", "PendSV_IRQn init");
    hal_nvic_init(&nvic_cfg);
    hal_nvic_global_irq_enable();
}

void nvic_deinit(Nvic* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void nvic_destroy(Nvic* self) {
    if (self != NULL) {
        nvic_deinit(self);
        os_free(self);
    }
}


// register method
bool nvic_register(Nvic* self, nvic_irq_num irq_num, nvic_handler_t handler, void *arg) {
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
    uint32_t key = arch_irq_lock();
    if ((self->irq_table + irq_num)->handler == NULL) {
        (self->irq_table + irq_num)->handler = handler;
    }
    (self->irq_table + irq_num)->arg = arg;
    (self->irq_table + irq_num)->registered = true;
    // 使能 NVIC 对应中断（假设已设置优先级）
    hal_nvic_enable_irq((IRQx + irq_num)->irqn);
    arch_irq_unlock(key);
    return true;
}
// unregister method
void nvic_unregister(Nvic* self, nvic_irq_num irq_num) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ) {
        return;
    }
    uint32_t key = arch_irq_lock();
    (self->irq_table + irq_num)->handler = NULL;
    (self->irq_table + irq_num)->arg = NULL;
    (self->irq_table + irq_num)->bottom_sem = NULL;
    (self->irq_table + irq_num)->registered = false;
    // 可选：禁用 NVIC 中断
    hal_nvic_disable_irq((IRQx + irq_num)->irqn);
    arch_irq_unlock(key);
}
// attach_semaphore method
void nvic_attach_semaphore(Nvic* self, nvic_irq_num irq_num, Semaphore *sem) {
    if (irq_num >= MAX_IRQ || sem == NULL)
        return;
    uint32_t key = arch_irq_lock();
    (self->irq_table + irq_num)->bottom_sem = sem;
    arch_irq_unlock(key);
}
// set_priority method
void nvic_set_priority(Nvic* self, nvic_irq_num irq_num, nvic_priority_t preempt_priority, uint8_t sub_priority) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ) {
        return;
    }
    hal_nvic_set_priority((IRQx + irq_num)->irqn, preempt_priority, sub_priority);
    hal_nvic_enable_irq((IRQx + irq_num)->irqn);
    LOG_DEBUG("nvic", "irq %s, priority (%d, %d)", (IRQx + irq_num)->irq_name, preempt_priority, sub_priority);
}
// dispatch method
void nvic_dispatch(Nvic* self, nvic_irq_num irq_num) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ)
        return;

    nvic_irq_t *irq = self->irq_table + irq_num;
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
        irq = (nvic_irq_t *)GET_NODE(irq)->next;
    }
}

void dispatch(nvic_irq_num irq_num) {
    nvic_dispatch(gloable_nvic, irq_num);
}


void SysTick_Handler(void) {
    nvic_dispatch(gloable_nvic, SYSTIC_IRQ);
}
void USART1_IRQHandler() {
    nvic_dispatch(gloable_nvic, USART1_IRQ);
}
void UART4_IRQHandler() {
    nvic_dispatch(gloable_nvic, USART4_IRQ);
}
void USART3_IRQHandler() {
    nvic_dispatch(gloable_nvic, USART3_IRQ);
}
void TIM2_IRQHandler() {
    nvic_dispatch(gloable_nvic, TIM2_IRQ);
}
void EXTI0_IRQHandler() {
    nvic_dispatch(gloable_nvic, EXTI0_IRQ);
}
void EXTI1_IRQHandler() {
    nvic_dispatch(gloable_nvic, EXTI1_IRQ);
}

void EXTI9_5_IRQHandler() {
    nvic_dispatch(gloable_nvic, EXTI5_9_IRQ);
}

void EXTI15_10_IRQHandler() {
    nvic_dispatch(gloable_nvic, EXTI10_15_IRQ);
}

void I2C1_EV_IRQHandler() {
    nvic_dispatch(gloable_nvic, I2C1_EV_IRQ);
}
void I2C1_ER_IRQHandler() {
    nvic_dispatch(gloable_nvic, I2C1_ER_IRQ);
}
void SPI1_IRQHandler() {
    nvic_dispatch(gloable_nvic, SPI1_IRQ);
}
void OTG_FS_IRQHandler(void)
{

}
void NMI_Handler(void)
{
    while (1)
    {
    }
}



void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    fault_info_t fault;
    hal_fault_diag_decode(&fault);
    while (1)
    {
        __WFE();
    }
}


void DebugMon_Handler(void)
{

}




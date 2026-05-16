#include "nvic.h"
#include <stdio.h>
#include "../../common/util.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"

static uint32_t nvic_encode_priority(Nvic* self, uint32_t preempt_priority, uint32_t sub_priority);

static bool nvic_register(Nvic* self, nvic_irq_num irq_num, nvic_handler_t handler, void *arg);
static void nvic_unregister(Nvic* self, nvic_irq_num irq_num);
static void nvic_attach_semaphore(Nvic* self, nvic_irq_num irq_num, Semaphore *sem);
static void nvic_set_priority(Nvic* self, nvic_irq_num irq_num, uint32_t priority);
static void nvic_dispatch(Nvic* self, nvic_irq_num irq_num);

// 析构函数声明
static void nvic_destroy(Nvic* self);
Nvic *gloable_nvic = NULL;
// TODO: 初始化数据成员
static const NvicFun nvic_fun = {
    .destroy = nvic_destroy,
	.register_handler = nvic_register,
	.unregister_handler = nvic_unregister,
	.attach_semaphore = nvic_attach_semaphore,
	.set_priority = nvic_set_priority,
    .encode_priority = nvic_encode_priority,
};
static const IRQn_Type IRQx[] = {
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
    DMA1_Stream0_IRQn,
    DMA1_Stream1_IRQn,
    DMA1_Stream2_IRQn,
    DMA1_Stream3_IRQn,
    DMA1_Stream4_IRQn,
    DMA1_Stream5_IRQn,
    DMA1_Stream6_IRQn,
    DMA1_Stream7_IRQn,
    DMA2_Stream0_IRQn,
    DMA2_Stream1_IRQn,
    DMA2_Stream2_IRQn,
    DMA2_Stream3_IRQn,
    DMA2_Stream4_IRQn,
    DMA2_Stream5_IRQn,
    DMA2_Stream6_IRQn,
    DMA2_Stream7_IRQn,
    ADC_IRQn,
    CAN1_TX_IRQn,
    CAN1_RX0_IRQn,
    CAN1_RX1_IRQn,
    CAN1_SCE_IRQn,
    CAN2_TX_IRQn,
    CAN2_RX0_IRQn,
    CAN2_RX1_IRQn,
    CAN2_SCE_IRQn,
    I2C1_EV_IRQn,
    I2C1_ER_IRQn,
    I2C2_EV_IRQn,
    I2C2_ER_IRQn,
    I2C3_EV_IRQn,
    I2C3_ER_IRQn,
    SPI1_IRQn,
    SPI2_IRQn,
    SPI3_IRQn,
    SDIO_IRQn,
        //FSMC_IRQ,
    TIM1_BRK_TIM9_IRQn,
    TIM1_UP_TIM10_IRQn,
    TIM1_TRG_COM_TIM11_IRQn,
    TIM1_CC_IRQn,
    TIM2_IRQn,
    TIM3_IRQn,
    TIM4_IRQn,
    TIM5_IRQn,
    TIM6_DAC_IRQn,
    TIM7_IRQn,
    TIM8_BRK_TIM12_IRQn,
    TIM8_UP_TIM13_IRQn,
    TIM8_TRG_COM_TIM14_IRQn,
    TIM8_CC_IRQn,
    RTC_Alarm_IRQn,
    RTC_WKUP_IRQn,
    OTG_FS_IRQn,
    UsageFault_IRQn,
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
    LOG_DEBUG("Nvic","Nvic_init");
    self->fun = &(nvic_fun);
    // TODO: 初始化数据成员
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
static bool nvic_register(Nvic* self, nvic_irq_num irq_num, nvic_handler_t handler, void *arg) {
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
static void nvic_unregister(Nvic* self, nvic_irq_num irq_num) {
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
static void nvic_attach_semaphore(Nvic* self, nvic_irq_num irq_num, Semaphore *sem) {
    if (irq_num >= MAX_IRQ || sem == NULL)
        return;
    DISABLE_IRQ;
    (self->irq_table + irq_num)->bottom_sem = sem;
    ENABLE_IRQ;
}
// set_priority method
static void nvic_set_priority(Nvic* self, nvic_irq_num irq_num, uint32_t priority) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ) {
        return;
    }
    NVIC_SetPriority(IRQx[irq_num], priority);
    NVIC_EnableIRQ(IRQx[irq_num]);

}
// dispatch method
static void nvic_dispatch(Nvic* self, nvic_irq_num irq_num) {
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


// encode_priority method
static uint32_t nvic_encode_priority(Nvic* self, uint32_t preempt_priority, uint32_t sub_priority) {
    return NVIC_EncodePriority(NVIC_GetPriorityGrouping(), preempt_priority, sub_priority);
}

void SysTick_Handler(void) {
    nvic_dispatch(gloable_nvic, SYSTIC_IRQ);
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

void HardFault_Handler(void)
{
    while (1)
    {

    }
}

void MemManage_Handler(void)
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
    /* USER CODE BEGIN UsageFault_IRQn 0 */
    //uint32_t usfr = SCB->CFSR;   // 用法故障状态寄存器
    //uint32_t ufsr = SCB->UFSR;   // 用法故障状态（同 CFSR 的低 16 位）
    /* USER CODE END UsageFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

void SVC_Handler(void)
{

}


void DebugMon_Handler(void)
{

}




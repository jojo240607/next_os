#include "exti.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "../task/os_cb_task.h"


dev_init_override(exti_dev_init_impl);

// 析构函数声明
static void exti_destroy(Exti* self);

// TODO: 初始化数据成员
static const ExtiFun exti_fun = {
    .destroy = exti_destroy,

};
// 构造函数实现
Exti* exti_create(exti_config *conf) {
    Exti* obj = (Exti*)os_malloc(sizeof(Exti));
    if (obj) {
        memset(obj, 0, sizeof(Exti));
        exti_init(obj, conf);
    }
    return obj;
}


void exti_init(Exti* self, exti_config *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(exti_fun);
    // TODO: 初始化派生类特有成员

	def_dev_init(self) = exti_dev_init_impl;

    self->conf = conf;

}

void exti_deinit(Exti* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放

}
// 析构函数实现
static void exti_destroy(Exti* self) {
    if (self != NULL) {
        exti_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(exti_dev_init_impl) {
    // TODO: add dev_init method

    Exti *exti = (Exti *)self;
    //params
    self->irq_conf.priority = NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
    self->irq_conf.handler = exti_irq_handler_impl;
    self->irq_conf.semaphore = sem;
    self->irq_conf.arg = self;
    //exti->exit_irq_conf.irq_num = new_irq_num;

    if (pinmux_request_group(exti->conf->pin_conf, exti->conf->pin_size, &self->irq_conf) == PINMUX_ERROR) {
        LOG_ERROR("exti", "pinmux error");
    }

/*
    // 1. 使能SYSCFG时钟。配置IO映射时，必须使能此时钟（挂载在APB2总线上）
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // 2. 使能GPIOA和GPIOB的时钟
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    //RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // 3. 设置中断优先级分组（此函数在core_cm4.h中定义）
   // NVIC_SetPriorityGrouping(0x03); // 选择第3组：4位抢占优先级，0位子优先级

    // ---------- PA0 配置 (连接至 EXTI 线0) ----------

    uint16_t exti_line = exti->conf->exti_line;
    while (exti_line) {
        // 最低位 1 的索引（0~15）
        int bit = __builtin_ctz(exti_line);
        // 1. 将GPIOA设置为输入模式并清除上下拉
        exti->conf->gpio_type->MODER &= ~(3 << (bit * 2));     // 清除原有模式位，设为输入模式（0b00）
        exti->conf->gpio_type->PUPDR &= ~(3 << (bit * 2));     // 清除上下拉设置，既不上拉也不下拉
        // 2. 将PA0连接到EXTI线0。SYSCFG_EXTICR1的低4位（EXTICR1[3:0])设置为0即代表选择PA0
//    操作后，EXTI线0的中断源就是PA0引脚了
        SYSCFG->EXTICR[bit >> 2] &= ~(0xFUL << ((bit & 0x03) << 2));  // 清除原有设置，让EXTI0映射到PA0
        SYSCFG->EXTICR[bit >> 2] |= 1 << ((bit & 0x03) << 2);//SYSCFG_EXTICR2_EXTI6_PB;

// 3. 开启EXTI线0的中断功能，并设置为上升沿触发
        EXTI->IMR |= (0x1UL << bit);//EXTI_IMR_MR0;   // 使能EXTI线0的中断请求
        EXTI->RTSR |= (0x1UL << bit); // 允许EXTI线0的上升沿触发中断
        EXTI->FTSR &= ~(0x1UL << bit) ; // 禁止下降沿触发


        intc_irq_num new_irq_num = MAX_IRQ;
        if (bit < 5) {
            new_irq_num = EXTI0_IRQ + bit;
        } else if (bit < 10) {
            new_irq_num = EXTI5_9_IRQ;
        } else {
            new_irq_num = EXTI10_15_IRQ;
        }
        if (new_irq_num != exti->exit_irq_conf.irq_num) {
            exti->exit_irq_conf.priority = NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x02, 0x00);
            exti->exit_irq_conf.handler = exti_irq_handler_impl;
            exti->exit_irq_conf.irq_num = new_irq_num;
            if (!self->fun->attach_irq(self, &exti->exit_irq_conf, sem)) {
                LOG_ERROR("timer", "attach irq %d error", exti->exit_irq_conf.irq_num);
            }
        }
        // 清除最低位的 1
        exti_line &= exti_line - 1;
    }
*/
}

// irq_handler method
bool exti_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    Exti *exti = (Exti *)arg;
    //params , void *arg

    // 检查是否是EXTI线0的中断挂起标志
    while (EXTI->PR) {
        // 最低位 1 的索引（0~15）
        uint32_t bit = __builtin_ctz(EXTI->PR);
        //printf("Need to process bit %d\n", bit);
        GET_DEVICE(exti)->irq_conf.semaphore->sem_event = OS_EVENT_EXTI0 + bit;
        //  清除中断标志位（写1清零）
        EXTI->PR |= (1 << bit);
    }
    return true;
}



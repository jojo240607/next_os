#include "intc.h"
#include <stdio.h>
#include "../common/util.h"
#include "../common/linear_pool.h"

static void intc_register(Intc* self, intc_irq_num irq_num, intc_handler_t handler, void *arg);
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
static intc_irq_t irq_table[MAX_IRQ];

// 构造函数实现
Intc* intc_create() {
    Intc* obj = (Intc*)os_malloc(sizeof(Intc));
    if (obj) {
        memset(obj, 0, sizeof(Intc));
        intc_init(obj);
    }
    return obj;
}

void intc_init(Intc* self) {
    self->fun = &(intc_fun);
    // TODO: 初始化数据成员
    //self->irq_tab = irq_table;
    for (uint32_t i = 0; i < MAX_IRQ; i++) {
        irq_table[i].handler = NULL;
        irq_table[i].arg = NULL;
        irq_table[i].bottom_sem = NULL;
        irq_table[i].registered = 0;
    }

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
static void intc_register(Intc* self, intc_irq_num irq_num, intc_handler_t handler, void *arg) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ || handler == NULL)
        return;
    // 临界区保护（关中断）
    DISABLE_IRQ;
    irq_table[irq_num].handler = handler;
    irq_table[irq_num].arg = arg;
    irq_table[irq_num].registered = 1;
    // 使能 NVIC 对应中断（假设已设置优先级）
    NVIC_EnableIRQ((IRQn_Type)irq_num);
    ENABLE_IRQ;
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
    irq_table[irq_num].handler = NULL;
    irq_table[irq_num].arg = NULL;
    irq_table[irq_num].bottom_sem = NULL;
    irq_table[irq_num].registered = 0;
    // 可选：禁用 NVIC 中断
    NVIC_DisableIRQ((IRQn_Type)irq_num);
    ENABLE_IRQ;
}
// attach_semaphore method
static void intc_attach_semaphore(Intc* self, intc_irq_num irq_num, Semaphore *sem) {
    if (irq_num >= MAX_IRQ || sem == NULL)
        return;
    DISABLE_IRQ;
    irq_table[irq_num].bottom_sem = sem;
    ENABLE_IRQ;
}
// set_priority method
static void intc_set_priority(Intc* self, intc_irq_num irq_num, uint32_t priority) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ)
        return;
    NVIC_SetPriority((IRQn_Type)irq_num, priority);
}
// dispatch method
static void intc_dispatch(Intc* self, intc_irq_num irq_num) {
    if (self == NULL) {
        return;
    }
    if (irq_num >= MAX_IRQ)
        return;

    intc_irq_t *irq = &irq_table[irq_num];
    if (irq == NULL) {
        return;
    }
    // 1. 执行上半部回调
    if (irq->handler) {
        if (irq->handler(irq->arg)) {
            // 2. 如果关联了下半部信号量，释放它（注意：此函数在中断中，应使用 from_isr 版本）
            if (irq->bottom_sem) {
                // 假设你的信号量有 semaphore_give_from_isr 函数
                // 并根据返回值决定是否需要请求调度
                irq->bottom_sem->fun->give(irq->bottom_sem);
            }
        }
    }


}

void dispatch(intc_irq_num irq_num) {
    intc_dispatch(gloable_intc, irq_num);
}


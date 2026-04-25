#ifndef INTC_H
#define INTC_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../scheduler/semaphore.h"

#define INTC_MAX_IRQ 82
#define GET_INTC(obj) ((Intc *)obj)
// 类声明
typedef struct _Intc Intc;
typedef struct _IntcFun IntcFun;
typedef struct _intc_irq_t intc_irq_t;
typedef enum _Intc_Irq intc_irq_num;
// 中断上半部回调函数原型（运行于中断上下文）
typedef void (*intc_handler_t)(void *arg);
enum _Intc_Irq {
    SYSTIC_IRQ = 0,
    USART4_IRQ = 1,
};
// 类成员函数结构
struct _IntcFun {
    void (*destroy)(Intc* self);
	void (*register_handler)(Intc* self, intc_irq_num irq_num, intc_handler_t handler, void *arg);
	void (*unregister_handler)(Intc* self, intc_irq_num irq_num);
	void (*attach_semaphore)(Intc* self, intc_irq_num irq_num, Semaphore *sem);
	void (*set_priority)(Intc* self, intc_irq_num irq_num, uint32_t priority);
};
// 中断控制块
struct _intc_irq_t {
    intc_handler_t handler;      // 上半部回调
    void *arg;                   // 回调参数
    Semaphore *bottom_sem;     // 关联的下半部信号量（可为 NULL）
    uint8_t registered;          // 是否已注册
};

// 类结构
struct _Intc {
    const IntcFun* fun;
    // TODO: 添加数据成员
    //intc_irq_t *irq_tab;
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
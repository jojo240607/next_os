#ifndef TCB_T_H
#define TCB_T_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "node.h"
#include "main.h"
//// 触发 PendSV 中断
#define Trigger_PendSV SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk
#define DISABLE_IRQ __disable_irq()
#define ENABLE_IRQ __enable_irq()

#define GET_TCB_T(obj) ((Tcb_t *)obj)
// 类声明
typedef struct _Tcb_t Tcb_t;
typedef struct _Tcb_tFun Tcb_tFun;
typedef enum _Tcb_tState Tcb_tState;
// 类成员函数结构
struct _Tcb_tFun {
    void (*destroy)(Tcb_t* self);
    void (*os_sleep)(Tcb_t* self, uint32_t ms);
};

enum _Tcb_tState {
    TCB_STATER_READY = 0x10,
    TCB_STATER_RUNNING,
    TCB_STATER_BLOCKED,
    TCB_STATER_DELAYED,
    TCB_STATER_DESTROY,
};
// 类结构
struct _Tcb_t {
    Node base;
    const Tcb_tFun* fun;
    // TODO: 添加数据成员
    const char *name;
    uint16_t tid;
    uint16_t priority;
    Tcb_tState state;
    uint32_t delay_ticks;   // 剩余等待节拍数
    uint32_t sp;
    size_t strack_size;
    uint32_t strack[];
};

// 构造函数声明
Tcb_t* tcb_t_create();
void tcb_t_init(Tcb_t* self);

// 析构函数声明
void tcb_t_deinit(Tcb_t* self);

#endif // TCB_T_H
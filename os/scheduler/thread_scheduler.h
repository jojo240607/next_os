#ifndef THREAD_SCHEDULER_H
#define THREAD_SCHEDULER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../common/queue.h"
#include "tcb_t.h"
//#define MAX_PRIORITY 32

#define GET_THREAD_SCHEDULER(obj) ((Thread_scheduler *)obj)
// 类声明
typedef struct _Thread_scheduler Thread_scheduler;
typedef struct _Thread_schedulerFun Thread_schedulerFun;
// 类成员函数结构
struct _Thread_schedulerFun {
    void (*destroy)(volatile Thread_scheduler* self);
    volatile Tcb_t *(*create_thread)(volatile Thread_scheduler* self, const char *name, thread_conf_t *entry_s);
	void (*start)(volatile Thread_scheduler* self);
	void (*delay_ticks)(volatile Thread_scheduler* self);
	void (*add_readly_list)(volatile Thread_scheduler* self, volatile Tcb_t *tcb, bool protected);

};

typedef enum : uint8_t {
    // Handler模式(中断/异常)：硬件强制使用MSP，SPSEL位被忽略。
    // 以下值描述的是退出异常，返回Thread模式时的状态。
    CONTROL_THREAD_MSP_PRIV   = 0x00, // 线程模式 + 主堆栈(MSP) + 特权级    (nPRIV=0, SPSEL=0)
    CONTROL_THREAD_PSP_PRIV   = 0x02, // 线程模式 + 进程堆栈(PSP) + 特权级  (nPRIV=0, SPSEL=1)
    CONTROL_THREAD_MSP_UNPRIV = 0x01, // 线程模式 + 主堆栈(MSP) + 非特权级  (nPRIV=1, SPSEL=0)
    CONTROL_THREAD_PSP_UNPRIV = 0x03, // 线程模式 + 进程堆栈(PSP) + 非特权级 (nPRIV=1, SPSEL=1)
} control_thread_state_t;

// 类结构
struct _Thread_scheduler {
    const Thread_schedulerFun* fun;
    // TODO: 添加数据成员
    volatile uint32_t priority_bitmap;                    // 位图，标记哪些优先级有就绪任务
    Queue *priority_list[THREAD_PRIORITY_MAX];          // 每个优先级的就绪任务链表
    Queue *delay_list;
    Queue *destroy_list;   /* 待销毁 TCB 队列 */
    uint16_t tid_num;
    volatile Tcb_t *current_thread;
};

// 构造函数声明
Thread_scheduler* thread_scheduler_create();
void thread_scheduler_init(volatile Thread_scheduler* self);

// 析构函数声明
void thread_scheduler_deinit(volatile Thread_scheduler* self);
void thread_scheduler_switch_context(volatile Thread_scheduler* self);

extern volatile Tcb_t *gloable_current_tcb;
extern volatile Thread_scheduler *global_thread_scheduler;
#endif // THREAD_SCHEDULER_H
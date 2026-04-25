#ifndef THREAD_SCHEDULER_H
#define THREAD_SCHEDULER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../common/queue.h"
#include "tcb_t.h"
#define MAX_PRIORITY 32


#define GET_THREAD_SCHEDULER(obj) ((Thread_scheduler *)obj)
// 类声明
typedef struct _Thread_scheduler Thread_scheduler;
typedef struct _Thread_schedulerFun Thread_schedulerFun;
// 类成员函数结构
struct _Thread_schedulerFun {
    void (*destroy)(Thread_scheduler* self);
    Tcb_t *(*create_thread)(Thread_scheduler* self, const char *name, Entry_t *entry_s);

	void (*start)(Thread_scheduler* self);

	void (*delay_ticks)(Thread_scheduler* self);
	void (*add_readly_list)(Thread_scheduler* self, Tcb_t *tcb);

};
// 类结构
struct _Thread_scheduler {
    const Thread_schedulerFun* fun;
    // TODO: 添加数据成员
    //Queue *run_queue; //运行线程队列
    uint32_t priority_bitmap;                    // 位图，标记哪些优先级有就绪任务
    Queue *priority_list[MAX_PRIORITY];          // 每个优先级的就绪任务链表（可简化成单任务）
    Queue *delay_list;
    Queue *destory_list;
    uint16_t tid_num;
    Tcb_t *current_thread;
};

// 构造函数声明
Thread_scheduler* thread_scheduler_create();
void thread_scheduler_init(Thread_scheduler* self);

// 析构函数声明
void thread_scheduler_deinit(Thread_scheduler* self);
void thread_scheduler_switch_context(Thread_scheduler* self);

extern uint32_t *gloable_current_stack;
extern Thread_scheduler *global_thread_scheduler;
#endif // THREAD_SCHEDULER_H
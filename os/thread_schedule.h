#ifndef THREAD_SCHEDULE_H
#define THREAD_SCHEDULE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "queue.h"
#include "tcb_t.h"

#define MAX_PRIORITY 32


#define GET_THREAD_SCHEDULE(obj) ((Thread_schedule *)obj)
// 类声明
typedef struct _Thread_schedule Thread_schedule;
typedef struct _Thread_scheduleFun Thread_scheduleFun;
// 类成员函数结构
struct _Thread_scheduleFun {
    void (*destroy)(Thread_schedule* self);
	void (*set_priority_ready)(Thread_schedule* self, uint8_t p);
	uint8_t (*get_highest_priority)(Thread_schedule* self);
	void (*clear_priority_ready)(Thread_schedule* self, uint8_t p);

	void (*delay_ticks)(Thread_schedule* self);

};
// 类结构
struct _Thread_schedule {
    const Thread_scheduleFun* fun;
    // TODO: 添加数据成员
    uint32_t priority_bitmap;                    // 位图，标记哪些优先级有就绪任务
    Queue *priority_list[MAX_PRIORITY];          // 每个优先级的就绪任务链表（可简化成单任务）
    Queue *delay_list;
    Tcb_t *current;
};

// 构造函数声明
Thread_schedule* thread_schedule_create();
void thread_schedule_init(Thread_schedule* self);

// 析构函数声明
void thread_schedule_deinit(Thread_schedule* self);
void thread_schedule_context_switch(Thread_schedule* self);

extern Thread_schedule *gloable_thread_schedule;
#endif // THREAD_SCHEDULE_H
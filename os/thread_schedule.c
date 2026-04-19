#include "thread_schedule.h"
#include <stdio.h>

static void thread_schedule_delay_ticks(Thread_schedule* self);

static void thread_schedule_clear_priority_ready(Thread_schedule* self, uint8_t p);

static void thread_schedule_set_priority_ready(Thread_schedule* self, uint8_t p);
static uint8_t thread_schedule_get_highest_priority(Thread_schedule* self);

// 析构函数声明
static void thread_schedule_destroy(Thread_schedule* self);

// TODO: 初始化数据成员
static const Thread_scheduleFun thread_schedule_fun = {
    .destroy = thread_schedule_destroy,
	.set_priority_ready = thread_schedule_set_priority_ready,
	.get_highest_priority = thread_schedule_get_highest_priority,
	.clear_priority_ready = thread_schedule_clear_priority_ready,
	.delay_ticks = thread_schedule_delay_ticks,
};

Thread_schedule *gloable_thread_schedule = NULL;
// 构造函数实现
Thread_schedule* thread_schedule_create() {
    Thread_schedule* obj = (Thread_schedule*)malloc(sizeof(Thread_schedule));
    if (obj) {
        memset(obj, 0, sizeof(Thread_schedule));
        thread_schedule_init(obj);
    }
    return obj;
}

void thread_schedule_init(Thread_schedule* self) {
    self->fun = &(thread_schedule_fun);
    // TODO: 初始化数据成员

}

void thread_schedule_deinit(Thread_schedule* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void thread_schedule_destroy(Thread_schedule* self) {
    if (self != NULL) {
        thread_schedule_deinit(self);
        free(self);
    }
}

// set_priority_ready method
static void thread_schedule_set_priority_ready(Thread_schedule* self, uint8_t p) {
    if (NULL == self) {
        return;
    }
    // 将任务加入 priority_list[p] 链表（略）
    if (self->priority_list[p]->size > 0) {
        self->priority_bitmap |= (1 << p);
        GET_TCB_T(self->priority_list[p]->head)->state = TCB_STATER_READY;
    }
}
// clear_priority_ready method
static void thread_schedule_clear_priority_ready(Thread_schedule* self, uint8_t p) {
    if (NULL == self) {
        return;
    }
    if (self->priority_list[p]->size == 0) {
        self->priority_bitmap &= ~(1 << p);
    }
}
// get_highest_priority method
static uint8_t thread_schedule_get_highest_priority(Thread_schedule* self) {
    if (NULL == self) {
        return 0;
    }
    return __builtin_clz(self->priority_bitmap);
}





// delay_ticks method
static void thread_schedule_delay_ticks(Thread_schedule* self) {
    if (NULL == self) {
        return;
    }
    // 遍历延时队列（或所有任务），将 delay_ticks 减 1
    Tcb_t *delay_task = self->delay_list->head;
    Tcb_t *prev = NULL;
    while (delay_task != NULL) {
        if (delay_task->delay_ticks > 0) {
            delay_task->delay_ticks--;
            if (delay_task->delay_ticks == 0) {
                // 延时结束，将任务移回就绪队列
                delay_task->state = TCB_STATER_READY;
                //remove_from_delay_list(task);
                if (prev == NULL) {
                    self->delay_list->head = GET_NODE(delay_task)->next;
                    self->delay_list->size--;
                } else {
                    GET_NODE(prev)->next = GET_NODE(delay_task)->next;
                    self->delay_list->size--;
                }
                self->priority_list[delay_task->priority]->fun->enqueue(self->priority_list[delay_task->priority], GET_NODE(delay_task));
                self->fun->set_priority_ready(self, delay_task->priority);
                // 如果该任务优先级高于当前任务，请求抢占
                if (delay_task->priority > self->current->priority) {
                    Trigger_PendSV;
                }
            }
        }
        prev = delay_task;
        delay_task = GET_TCB_T(GET_NODE(delay_task)->next);
    }

}

void thread_schedule_context_switch(Thread_schedule* self) {
    if (NULL == self) {
        return;
    }
    if (self->current->state == TCB_STATER_READY || self->current->state == TCB_STATER_RUNNING) {
        self->priority_list[self->current->priority]->fun->enqueue(self->priority_list[self->current->priority],
                                                                   GET_NODE(self->current));
    } else {

        thread_schedule_clear_priority_ready(self, self->current->priority);
        
    }

}


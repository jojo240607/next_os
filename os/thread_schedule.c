#include "thread_schedule.h"
#include <stdio.h>

static void thread_schedule_create_thread(Thread_schedule* self, const char *name, Tcb_Entey *entey);

static void thread_schedule_add_readly_list(Thread_schedule* self, Tcb_t *tcb);

static void thread_schedule_delay_ticks(Thread_schedule* self);

static void thread_schedule_clear_priority_ready(Thread_schedule* self, uint8_t p);

static void thread_schedule_set_priority_ready(Thread_schedule* self, uint8_t p);
static uint8_t thread_schedule_get_highest_priority(Thread_schedule* self);

// 析构函数声明
static void thread_schedule_destroy(Thread_schedule* self);

// TODO: 初始化数据成员
static const Thread_scheduleFun thread_schedule_fun = {
    .destroy = thread_schedule_destroy,
	.delay_ticks = thread_schedule_delay_ticks,
	.add_readly_list = thread_schedule_add_readly_list,
	.create_thread = thread_schedule_create_thread,
};

Thread_schedule *gloable_thread_schedule = NULL;
uint32_t *gloable_current_stack = NULL;
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
    self->priority_bitmap = 0;
    for (uint8_t i = 0; i < MAX_PRIORITY; i++) {
        self->priority_list[i] = queue_create();
    }
    self->delay_list = queue_create();
    self->destory_list = queue_create();
    self->current = NULL;
}

void thread_schedule_deinit(Thread_schedule* self) {
    // TODO: 数据成员申请资源释放
    self->priority_bitmap = 0;
    self->current = NULL;
    for (uint8_t i = 0; i < MAX_PRIORITY; i++) {
        if (self->priority_list[i] != NULL) {
            Tcb_t *tcb = GET_TCB_T(self->priority_list[i]->fun->dequeue(self->priority_list[i]));
            while (tcb != NULL) {
                tcb->fun->destroy(tcb);
                tcb = GET_TCB_T(self->priority_list[i]->fun->dequeue(self->priority_list[i]));
            }
            self->priority_list[i]->fun->destroy(self->priority_list[i]);
        }
    }
    if (self->delay_list) {
        Tcb_t *tcb = GET_TCB_T(self->delay_list->fun->dequeue(self->delay_list));
        while (tcb != NULL) {
            tcb->fun->destroy(tcb);
            tcb = GET_TCB_T(self->delay_list->fun->dequeue(self->delay_list));
        }
        self->delay_list->fun->destroy(self->delay_list);
    }
    if (self->destory_list) {
        Tcb_t *tcb = GET_TCB_T(self->delay_list->fun->dequeue(self->delay_list));
        while (tcb != NULL) {
            tcb->fun->destroy(tcb);
            tcb = GET_TCB_T(self->delay_list->fun->dequeue(self->delay_list));
        }
        self->destory_list->fun->destroy(self->destory_list);
    }
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
                thread_schedule_add_readly_list(self, delay_task);
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
    DISABLE_IRQ;
    if (self->current->state == TCB_STATER_READY || self->current->state == TCB_STATER_RUNNING) {
        thread_schedule_add_readly_list(self, self->current);
    } else {
        thread_schedule_clear_priority_ready(self, self->current->priority);
        self->destory_list->fun->enqueue(self->destory_list, GET_NODE(self->current));
    }
    uint8_t highest_priority = thread_schedule_get_highest_priority(self);
    //判断当前优先级的任务队列是否为空，若为空则将标志位清空
    while (self->priority_list[highest_priority]->size == 0) {
        thread_schedule_clear_priority_ready(self, highest_priority);
        highest_priority = thread_schedule_get_highest_priority(self);
    }
    Tcb_t *next_tcb = GET_TCB_T(
            self->priority_list[highest_priority]->fun->dequeue(self->priority_list[highest_priority]));
    self->current = next_tcb;
    gloable_current_stack = &self->current->sp;
    ENABLE_IRQ;
}


// add_readly_list method
static void thread_schedule_add_readly_list(Thread_schedule* self, Tcb_t *tcb) {
    if (NULL == self) {
        return;
    }
    // 将任务放回就绪队列（根据优先级插入）
    self->priority_list[tcb->priority]->fun->enqueue(self->priority_list[tcb->priority], GET_NODE(tcb));
    thread_schedule_set_priority_ready(self, tcb->priority);
    // 如果该任务优先级高于当前任务，请求抢占
    if (tcb->priority > self->current->priority) {
        Trigger_PendSV;
    }
}


// create_thread method
static void thread_schedule_create_thread(Thread_schedule* self, const char *name, Tcb_Entey *entey) {
    // TODO: add create_thread method
    
}


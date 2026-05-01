#include "thread_scheduler.h"
#include <stdio.h>
#include "main.h"
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "../common/sys_time.h"

static inline void thread_scheduler_set_priority_ready(Thread_scheduler* self, uint8_t p);
static inline void thread_scheduler_clear_priority_ready(Thread_scheduler* self, uint8_t p);
static inline uint8_t thread_scheduler_get_highest_priority(Thread_scheduler* self);

static void thread_scheduler_delay_ticks(Thread_scheduler* self);
static void thread_scheduler_add_readly_list(Thread_scheduler* self, Tcb_t *tcb, bool protected);

static void thread_scheduler_start(Thread_scheduler* self);

static Tcb_t * thread_scheduler_create_thread(Thread_scheduler* self, const char *name, Entry_t *entry_s);
// 析构函数声明
static void thread_scheduler_destroy(Thread_scheduler* self);
static inline void thread_scheduler_thread_exit();

// 当前任务栈指针

uint32_t *gloable_current_stack = NULL;
Thread_scheduler *global_thread_scheduler = NULL;

// TODO: 初始化数据成员
static const Thread_schedulerFun thread_scheduler_fun = {
    .destroy = thread_scheduler_destroy,
	.create_thread = thread_scheduler_create_thread,
	.start = thread_scheduler_start,
	.delay_ticks = thread_scheduler_delay_ticks,
	.add_readly_list = thread_scheduler_add_readly_list,
};
// 构造函数实现
Thread_scheduler* thread_scheduler_create() {
    Thread_scheduler* obj = (Thread_scheduler*)os_malloc(sizeof(Thread_scheduler));
    if (obj) {
        memset(obj, 0, sizeof(Thread_scheduler));
        thread_scheduler_init(obj);
    }
    return obj;
}

void thread_scheduler_init(Thread_scheduler* self) {
    LOG_DEBUG("scheduler", "thread_scheduler_init");
    self->fun = &(thread_scheduler_fun);
    // TODO: 初始化数据成员
    self->priority_bitmap = 0;
    for (uint8_t i = 0; i < MAX_PRIORITY; i++) {
        self->priority_list[i] = queue_create();
    }
    self->delay_list = queue_create();
    self->destory_list = queue_create();
    self->tid_num = 1000;
    self->current_thread = NULL;
}

void thread_scheduler_deinit(Thread_scheduler* self) {
    // TODO: 数据成员申请资源释放
    self->priority_bitmap = 0;
    self->current_thread = NULL;
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
static void thread_scheduler_destroy(Thread_scheduler* self) {
    if (self != NULL) {
        thread_scheduler_deinit(self);
        os_free(self);
    }
}

// thread_create method
static Tcb_t *thread_scheduler_create_thread(Thread_scheduler* self, const char *name, Entry_t *entry_s) {
    if (NULL == self) {
        return NULL;
    }
    entry_s->exit = thread_scheduler_thread_exit;
    Tcb_t *new_thread = tcb_t_create(name, entry_s, entry_s->stack_size);
    new_thread->tid = self->tid_num++;
    thread_scheduler_add_readly_list(self, new_thread, true);
    //self->priority_list[entry_s->priority]->fun->enqueue(self->priority_list[entry_s->priority], GET_NODE(new_thread));
    return new_thread;
}

// 调度器：选择下一个任务，更新 pxCurrentTCB
void thread_scheduler_switch_context(Thread_scheduler* self) {
    if (NULL == self || self->current_thread == NULL) {
        return;
    }
    if (*(self->current_thread->stack_ptr + 1) != MAGIC_NUM) {
        while (1) {
            LOG_ERROR("scheduler", "%s stack out of bound", self->current_thread->name);
            //stack out bound
        }
    } else {
#if OS_STACK_DEBUG
        uint16_t left = 0;
        while (*(self->current_thread->stack_ptr + left + 1) == MAGIC_NUM) {
            left++;
        }
        self->current_thread->stack_left = left;
#endif
    }
    DISABLE_IRQ;
    if (self->current_thread->state == TCB_STATER_READY || self->current_thread->state == TCB_STATER_RUNNING) {
        thread_scheduler_add_readly_list(self, self->current_thread, true);
    } else if (TCB_STATER_TERMINATED == self->current_thread->state) {
        self->destory_list->fun->enqueue(self->destory_list, GET_NODE(self->current_thread));
    }

    uint8_t highest_priority = thread_scheduler_get_highest_priority(self);
    if (highest_priority > 31) {
        ENABLE_IRQ;
        return;
    }
    //判断当前优先级的任务队列是否为空，若为空则将标志位清空
    while (self->priority_list[highest_priority]->size == 0 && highest_priority < 32) {
        thread_scheduler_clear_priority_ready(self, highest_priority);
        highest_priority = thread_scheduler_get_highest_priority(self);
    }
    Tcb_t *next_tcb = GET_TCB_T(
            self->priority_list[highest_priority]->fun->dequeue(self->priority_list[highest_priority]));
    self->current_thread->run_time += getSystime()->systick - self->current_thread->start_time;
    //if (self->current_thread->need_print) {
    //    self->current_thread->need_print = false;
    //    LOG_DEBUG("scheduler", "thread %s -> thread %s, size %d", self->current_thread->name, next_tcb->name, self->priority_list[0]->size);
    //}
    self->current_thread = next_tcb;
    if (self->current_thread != NULL) {
        self->current_thread->start_time = getSystime()->systick;
        self->current_thread->state = TCB_STATER_RUNNING;
        gloable_current_stack = (uint32_t *)(&self->current_thread->sp);
    }
    ENABLE_IRQ;
}


// start method
static void thread_scheduler_start(Thread_scheduler* self) {
    LOG_DEBUG("scheduler", "thread_scheduler_start");
    if (NULL == self) {
        return;
    }
    uint8_t highest_priority = thread_scheduler_get_highest_priority(self);
    self->current_thread = GET_TCB_T(self->priority_list[highest_priority]->fun->dequeue(self->priority_list[highest_priority]));
    if (self->current_thread == NULL) {
        return;
    }
    __set_PSP( (uint32_t)self->current_thread->sp );
    // 设置 CONTROL 寄存器，选择使用 PSP
    __set_CONTROL( __get_CONTROL() | 0x2 );
    // 执行 ISB 指令确保立即生效
    __ISB();
    Trigger_PendSV;
    // 或者直接使用 svc 指令
    // 注意：永远不会返回到这里
    while(1);
}

static inline void thread_scheduler_thread_exit() {
    global_thread_scheduler->current_thread->state = TCB_STATER_TERMINATED;
    // 4. 触发 PendSV 完成切换
    Trigger_PendSV;
    // 5. 永不返回
    while(1);
}

// delay_ticks method
static void thread_scheduler_delay_ticks(Thread_scheduler* self) {
    if (NULL == self) {
        return;
    }
    // 遍历延时队列（或所有任务），将 delay_ticks 减 1
    Tcb_t *delay_task = GET_TCB_T(self->delay_list->head);
    Tcb_t *prev = NULL;
    uint8_t num = 0;
    while (delay_task != NULL) {
        if (delay_task->delay_ticks > 0) {
            delay_task->delay_ticks--;
            if (delay_task->delay_ticks == 0) {
                // 延时结束，将任务移回就绪队列
                delay_task->state = TCB_STATER_READY;
                //remove_from_delay_list(task);
                if (num == 0) {
                    self->delay_list->head = GET_NODE(delay_task)->next;
                    self->delay_list->size--;
                    if (self->delay_list->size < 2) {
                        self->delay_list->tail = self->delay_list->head;
                    }
                } else {
                    GET_NODE(prev)->next = GET_NODE(delay_task)->next;
                    self->delay_list->size--;
                    if (self->delay_list->size < 2) {
                        self->delay_list->tail = self->delay_list->head;
                    }
                    if (GET_NODE(delay_task) == self->delay_list->tail) {
                        self->delay_list->tail = GET_NODE(prev);
                    }
                }
                LOG_DEBUG("delay", "delay over, add_readly %s", delay_task->name);
                thread_scheduler_add_readly_list(self, delay_task, false);
            }
        }
        num++;
        prev = delay_task;
        delay_task = GET_TCB_T(GET_NODE(delay_task)->next);
        //while (delay_task->state != TCB_STATER_WAITING_MUTEX) {
        //    prev = delay_task;
        //    delay_task = GET_TCB_T(GET_NODE(delay_task)->next);
        //}
    }
    
}
// add_readly_list method
static void thread_scheduler_add_readly_list(Thread_scheduler* self, Tcb_t *tcb, bool protected) {
    if (NULL == self) {
        return;
    }
    // 将任务放回就绪队列（根据优先级插入）
    self->priority_list[tcb->priority]->fun->enqueue(self->priority_list[tcb->priority], GET_NODE(tcb));
    thread_scheduler_set_priority_ready(self, tcb->priority);
    // 如果该任务优先级高于当前任务，请求抢占
    if (!protected && self->current_thread != NULL && tcb->priority > self->current_thread->priority) {
        Trigger_PendSV;
    }
}

// set_priority_ready method
static inline void thread_scheduler_set_priority_ready(Thread_scheduler* self, uint8_t p) {
    if (NULL == self) {
        return;
    }
    // 将任务加入 priority_list[p] 链表（略）
    if (self->priority_list[p]->size > 0) {
        self->priority_bitmap |= (1 << p);//设置优先级 31 位对应优先级31， 0位 对应优先级 0
        GET_TCB_T(self->priority_list[p]->tail)->state = TCB_STATER_READY;
    }
}
// clear_priority_ready method
static inline void thread_scheduler_clear_priority_ready(Thread_scheduler* self, uint8_t p) {
    if (NULL == self) {
        return;
    }
    if (self->priority_list[p]->size == 0) {
        self->priority_bitmap &= ~(1 << p);
    }
}
// get_highest_priority method
static inline uint8_t thread_scheduler_get_highest_priority(Thread_scheduler* self) {
    //用于计算一个无符号整数的前导零个数  优先级31 返回值0， 优先级0 返回31
    return 31 - __builtin_clz(self->priority_bitmap);
}


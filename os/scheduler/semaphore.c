#include "semaphore.h"
#include <stdio.h>
#include "main.h"
#include "thread_scheduler.h"
#include "../common/linear_pool.h"

static void semaphore_take(Semaphore* self);
static void semaphore_give(Semaphore* self);

// 析构函数声明
static void semaphore_destroy(Semaphore* self);

// TODO: 初始化数据成员
static const SemaphoreFun semaphore_fun = {
    .destroy = semaphore_destroy,
	.take = semaphore_take,
	.give = semaphore_give,
};
// 构造函数实现
Semaphore* semaphore_create(uint8_t count) {
    Semaphore* obj = (Semaphore*)os_malloc(sizeof(Semaphore));
    if (obj) {
        memset(obj, 0, sizeof(Semaphore));
        semaphore_init(obj, count);
    }
    return obj;
}

void semaphore_init(Semaphore* self, uint8_t count) {
    self->fun = &(semaphore_fun);
    // TODO: 初始化数据成员
    self->count = count;
    self->wait_list = queue_create();
}

void semaphore_deinit(Semaphore* self) {
    // TODO: 数据成员申请资源释放
    if (self->wait_list != NULL) {
        Tcb_t *tcb = GET_TCB_T(self->wait_list->fun->dequeue(self->wait_list));
        while (tcb != NULL) {
            tcb->fun->destroy(tcb);
            tcb = GET_TCB_T(self->wait_list->fun->dequeue(self->wait_list));
        }
        self->wait_list->fun->destroy(self->wait_list);
    }
}

// 析构函数实现
static void semaphore_destroy(Semaphore* self) {
    if (self != NULL) {
        semaphore_deinit(self);
        os_free(self);
    }
}

// take method
static void semaphore_take(Semaphore* self) {
    if (NULL == self) {
        return;
    }

    DISABLE_IRQ;               // 进入临界区
    if (self->count > 0) {
        self->count--;
        ENABLE_IRQ;            // 快速路径，不阻塞
        return;
    }

    // 需要阻塞当前任务
    Tcb_t *current = global_thread_scheduler->current_thread;//pxCurrentTCB;
    current->state = TCB_STATER_BLOCKED;
    // 将当前任务插入信号量的等待队列尾部
    //insert_into_wait_list(&sem->wait_list, current);
    self->wait_list->fun->enqueue(self->wait_list, GET_NODE(current));
    ENABLE_IRQ;
    Trigger_PendSV;
}
// give method
static void semaphore_give(Semaphore* self) {
    if (NULL == self) {
        return;
    }
    DISABLE_IRQ;
    if (self->wait_list->size > 0) {
        // 有任务在等待：取出队首任务
        Tcb_t *task = GET_TCB_T(self->wait_list->fun->dequeue(self->wait_list));
        task->state = TCB_STATER_READY;
        // 将任务放回就绪队列（根据优先级插入）
        global_thread_scheduler->fun->add_readly_list(global_thread_scheduler, task);
    } else {
        self->count++;
    }
    ENABLE_IRQ;
}


#include "mutex.h"
#include <stdio.h>
#include "../common/linear_pool.h"
#include "thread_scheduler.h"
#include "../log/log.h"
#include "../driver/svc.h"

static uint8_t mutex_take(Mutex* self, uint32_t timeout_ms);
static uint8_t mutex_give(Mutex* self);

// 析构函数声明
static void mutex_destroy(Mutex* self);

// TODO: 初始化数据成员
static const MutexFun mutex_fun = {
    .destroy = mutex_destroy,
	.mutex_lock = mutex_take,
	.mutex_unlock = mutex_give,
};
// 构造函数实现
Mutex* mutex_create() {
    Mutex* obj = (Mutex*)os_malloc(sizeof(Mutex));
    if (obj) {
        memset(obj, 0, sizeof(Mutex));
        mutex_init(obj);
    }
    return obj;
}

void mutex_init(Mutex* self) {
    self->fun = &(mutex_fun);
    // TODO: 初始化数据成员
    self->owner = NULL;
    self->lock_count = 0;
    self->owner_original_prio = 0;
    self->wait_list = queue_create();
}

void mutex_deinit(Mutex* self) {
    // TODO: 数据成员申请资源释放
    if (self->wait_list) {
        self->wait_list->fun->destroy(self->wait_list);
    }
}

// 析构函数实现
static void mutex_destroy(Mutex* self) {
    if (self != NULL) {
        mutex_deinit(self);
        os_free(self);
    }
}

// take method
// 注意：timeout_ms 参数当前未实现，调用者必须传 OS_WAIT_FOREVER(0) 或接受永久阻塞。
// TODO: 在延时队列中加入超时唤醒机制。
static uint8_t mutex_take(Mutex* self, uint32_t timeout_ms) {
    uint32_t key = arch_irq_lock();
    volatile Tcb_t *curr = global_thread_scheduler->current_thread;
    // 情况1：互斥量空闲，直接获取
    if (self->owner == NULL) {
        self->owner = curr;
        self->lock_count = 1;
        self->owner_original_prio = curr->priority;  // 记录原优先级
        arch_irq_unlock(key);
        return 1;
    }
    // 情况2：已经持有，递归加锁
    if (self->owner == curr) {
        self->lock_count++;
        arch_irq_unlock(key);
        return 1;
    }

    // 情况3：互斥量被其他任务持有 -> 优先级继承
    volatile Tcb_t *owner = self->owner;
    if (curr->priority > owner->priority) {   // 当前任务优先级更高
        // 提升持有者优先级到当前任务优先级
        // 注意：需要重新插入就绪队列（因为优先级变了）
        //此处需要做特殊判断，若owner任务在执行态，需要从优先级列表中挪到另一个优先级，若处于未就绪态，则只需要改变其priority就可以
        //因为在任务切换的时候会自动放入就绪状态
        if (owner->state == TCB_STATER_READY || owner->state == TCB_STATER_RUNNING) {
            if (global_thread_scheduler->priority_list[owner->priority]->fun->dequeue_node(
                    global_thread_scheduler->priority_list[owner->priority], GET_NODE(owner)) != NULL) {
                owner->priority = curr->priority;
                global_thread_scheduler->fun->add_readly_list(global_thread_scheduler, owner, true);
            } else {
                LOG_ERROR("mutex", "need update priority, but not found !!!");
            }
        } else {
            owner->priority = curr->priority;
        }
    }
    // 挂起当前任务，加入等待队列
    curr->state = TCB_STATER_WAITING_MUTEX;
    self->wait_list->fun->enqueue(self->wait_list, GET_NODE(curr));
    arch_irq_unlock(key);
    Trigger_PendSV;   // 切换到其他任务
    //start_pendsv_user();
    //LOG_DEBUG("mutex", "mutex_take 2");
    // 被唤醒后，重新获取互斥量所有权
    // 注意：此时互斥量已由原持有者释放，并可能已经转移给当前任务
    key = arch_irq_lock();
 //   LOG_DEBUG("mutex", "wake up %s", curr->name);
    //被唤醒后第一次的状态应该是TCB_STATER_WAITING_MUTEX
    if (curr->state == TCB_STATER_RUNNING) {
        // 成功获得互斥量
        self->owner = curr;
        self->lock_count = 1;
        self->owner_original_prio = curr->priority;  // 覆盖为实际优先级
        arch_irq_unlock(key);
        LOG_DEBUG("mutex", "wake up %s right", curr->name);
        return 1;
    } else {
        LOG_ERROR("mutex", "lock fail, owner %s state %d",owner->name, curr->state);
        // 超时或被中断唤醒
        arch_irq_unlock(key);
        return 0;
    }
}
// give method
static uint8_t mutex_give(Mutex* self) {
    uint32_t key = arch_irq_lock();
    if (self->owner != global_thread_scheduler->current_thread) {
        // 非法释放：不是持有者
        LOG_ERROR("mutex", "Illegal give owner %s current is %s", self->owner->name, global_thread_scheduler->current_thread->name);
        arch_irq_unlock(key);
        return 0;
    }

    self->lock_count--;
    if (self->lock_count > 0) {
        // 递归锁未完全释放
        arch_irq_unlock(key);
        return 1;
    }

    // 恢复所有者原来的优先级（如果曾被提升）
    volatile Tcb_t *owner = self->owner;
    if (owner->priority != owner->original_priority) {
        owner->priority = owner->original_priority;
        // 重新插入就绪队列
        //当前任务不需要手动插入队列，只需要调整state就可以，任务切换时会根据当前任务的state决定是都插入就绪队列
        owner->state = TCB_STATER_READY;
    }

    // 检查等待队列
    if (self->wait_list->size == 0) {
        // 没有等待者，直接清空所有者
        self->owner = NULL;
        arch_irq_unlock(key);
        return 1;
    }

    // 有等待任务：取出等待队列头（优先级最高）
    Tcb_t *new_owner = GET_TCB_T(self->wait_list->fun->dequeue(self->wait_list));
    new_owner->state = TCB_STATER_READY;
    // 直接转移所有权（不经过调度，因为还在临界区）
    self->owner = new_owner;
    self->lock_count = 1;
    self->owner_original_prio = new_owner->priority;
    // 新任务不是当前任务，需要手动插入插入就绪队列
    LOG_DEBUG("mutex", "add wait unlock owner %s", new_owner->name);
    global_thread_scheduler->fun->add_readly_list(global_thread_scheduler, new_owner, true);
    // TODO: 超时机制实现后，此处需清除新拥有者的超时定时器
    arch_irq_unlock(key);
    Trigger_PendSV;
    return 1;
}


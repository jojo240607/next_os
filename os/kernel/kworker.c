/**
 * 内核工作线程 — 工作队列的消费者
 *
 *   循环: sem_take(wake) → 取出所有 work → 逐个跑完状态机
 *
 *   驱动提交: kwork_submit(kw, &work) — 入队 + 唤醒线程
 */
#include "kworker.h"
#include "../common/linear_pool.h"
#include "../common/util.h"
#include "../log/log.h"

static void kworker_destroy(kworker_t *self);

task_init_override(kw_init);
task_thread_override(kw_thread);
task_start_override(kw_start);


kworker_t *gloable_kworker = NULL;

/* ════ 构造 / 析构 ════ */

kworker_t* kworker_create(const task_into_t *info)
{
    kworker_t *obj = os_malloc(sizeof(kworker_t));
    if (obj) { memset(obj, 0, sizeof(*obj)); kworker_init(obj, info); }
    return obj;
}

void kworker_init(kworker_t *self, const task_into_t *info)
{
    task_init(&self->base, info);
    GET_TASK_VTABLE(self)->task_init   = kw_init;
    GET_TASK_VTABLE(self)->task_thread = kw_thread;
    GET_TASK_VTABLE(self)->task_start  = kw_start;

    self->queue = queue_create();
    self->wake  = semaphore_create(0);
    gloable_kworker = self;
}

static void kworker_destroy(kworker_t *self)
{
    if (self) { task_deinit(GET_TASK(self)); os_free(self); }
}

/* ════ Task 虚函数 ════ */

task_init_override(kw_init)
{
    LOG_DEBUG("kworker", "init");
    (void)parent;
}

task_start_override(kw_start)
{
    LOG_DEBUG("kworker", "started");
    (void)self;
}

task_thread_override(kw_thread)
{
    kworker_t *kw = (kworker_t *)self->parent;

    while (1) {
        kw->wake->fun->take(kw->wake);
        kwork_t *work = (kwork_t *)kw->queue->fun->dequeue(kw->queue);
        if (work) {
            int next = work->fn(work);
            if (next > 0) {
                /* 状态机未完成: 重新入队, 等中断 wake */
                uint32_t key = arch_irq_lock();
                kw->queue->fun->enqueue(kw->queue, (Node *)work);
                arch_irq_unlock(key);
            }
        }
    }
}

/* ════ 提交 ════ */

void kwork_submit(kworker_t *kw, kwork_t *work)
{
    if (!kw || !work || !work->fn) return;

    uint32_t key = arch_irq_lock();
    kw->queue->fun->enqueue(kw->queue, (Node *)work);
    arch_irq_unlock(key);

    kw->wake->fun->give(kw->wake);
}

/**
 * 内核工作线程 — 唯一的系统级工作队列消费者
 *
 * 驱动通过 kwork_submit() 将工作项放入队列，
 * 内核线程 (优先级最高) 自动出队并循环执行状态机直至完成。
 *
 * 驱动自己管理完成通知 (通常是在状态机最后一步 sem_give)。
 */
#ifndef KWORKER_H
#define KWORKER_H

#include "../common/queue.h"
#include "../task/task.h"
#include "kwork.h"

typedef struct kworker {
    Task       base;        /* 继承 Task */
    Queue     *queue;       /* 工作队列 */
    Semaphore *wake;        /* 有新工作时唤醒 */
} kworker_t;

extern kworker_t *gloable_kworker;

/**
 * 提交工作项到内核线程队列 (不等待)
 * 完成通知由驱动在 fn 最后一步自行处理。
 */
void kwork_submit(kworker_t *kw, kwork_t *work);

kworker_t* kworker_create(const task_into_t *info);
void       kworker_init(kworker_t *self, const task_into_t *info);

#endif

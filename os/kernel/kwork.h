/**
 * 内核工作项 + 通用状态机 — 纯基础设施，不耦合任何驱动
 *
 * 工作项:
 *   - 节点 (Node):   用于挂入工作队列 (Queue)
 *   - 状态机函数:    每个状态返回下一状态或 DONE
 *   - 上下文指针:    驱动自定义数据 (通常栈分配)
 *
 * 状态机返回值:
 *   > 0  ─ 下一状态编号
 *     0  ─ 完成 (KWORK_DONE)
 *   < 0  ─ 错误
 *
 * ─── 驱动使用示例 ───
 *
 *   #include "kernel/kwork.h"
 *
 *   struct my_ctx { int id; uint8_t *buf; Semaphore *done; };
 *
 *   int my_state_fn(kwork_t *w) {
 *       struct my_ctx *c = (struct my_ctx *)w->ctx;
 *       KWORK_BEGIN();
 *       KWORK_STATE(1):
 *           hal_do_step1(c->id);
 *           KWORK_NEXT(2);
 *       KWORK_STATE(2):
 *           hal_do_step2(c->buf);
 *           if (c->done) sem_give(c->done);
 *           KWORK_DONE();
 *       KWORK_END();
 *   }
 *
 *   // 调用:
 *   struct my_ctx ctx = {.id = 0, .buf = buf, .done = sem};
 *   kwork_t work;
 *   kwork_init(&work, my_state_fn, &ctx);
 *   kwork_submit(gloable_kworker, &work);
 *   sem_take(ctx.done);   // 等待完成
 */
#ifndef KWORK_H
#define KWORK_H

#include "../common/node.h"
#include "../scheduler/semaphore.h"

typedef struct kwork kwork_t;

/** 状态机函数 */
typedef int (*kwork_fn_t)(kwork_t *work);

struct kwork {
    Node       node;       /* 必须为第一个成员 ─ 用于 Queue */
    kwork_fn_t fn;         /* 状态机函数 */
    void      *ctx;        /* 驱动上下文 */
    int        state;      /* 当前状态 */
};

/* ─── 状态机宏 ─── */

#define KWORK_BEGIN(work)    switch (work->state) {
#define KWORK_STATE(n)   case (n):
#define KWORK_NEXT(s)    do { work->state = (s); return (s); } while(0)
#define KWORK_DONE()     return 0
#define KWORK_END()      default: return -1; }

/* ─── 初始化 ─── */

static inline void kwork_init(kwork_t *w, kwork_fn_t fn, void *ctx)
{
    w->node.next = NULL;
    w->fn    = fn;
    w->ctx   = ctx;
    w->state = 0;
}

#endif

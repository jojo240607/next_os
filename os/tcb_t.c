#include "tcb_t.h"
#include <stdio.h>
#include "thread_schedule.h"

// 析构函数声明
static void tcb_t_destroy(Tcb_t* self);
static void tcb_t_sleep(Tcb_t* self, uint32_t ms);
// TODO: 初始化数据成员
static const Tcb_tFun tcb_t_fun = {
    .destroy = tcb_t_destroy,
    .os_sleep = tcb_t_sleep,
};
// 构造函数实现
Tcb_t* tcb_t_create() {
    Tcb_t* obj = (Tcb_t*)malloc(sizeof(Tcb_t));
    if (obj) {
        memset(obj, 0, sizeof(Tcb_t));
        tcb_t_init(obj);
    }
    return obj;
}

void tcb_t_init(Tcb_t* self) {
    self->fun = &(tcb_t_fun);
    // TODO: 初始化数据成员

}

void tcb_t_deinit(Tcb_t* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void tcb_t_destroy(Tcb_t* self) {
    if (self != NULL) {
        tcb_t_deinit(self);
        free(self);
    }
}

static void tcb_t_sleep(Tcb_t* self, uint32_t ms) {
    uint32_t ticks = ms;//ms_to_ticks(ms);   // 毫秒转节拍数
    DISABLE_IRQ;
    self->delay_ticks = ticks;
    self->state = TCB_STATER_DELAYED;          // 延时阻塞状态
    // 可选：将任务插入一个专门的延时队列（按唤醒时间排序）
    gloable_thread_schedule->delay_list->fun->enqueue(gloable_thread_schedule->delay_list, GET_NODE(self));
    // 按 delay_ticks 升序插入
    ENABLE_IRQ;
    // 触发调度，切换到下一个就绪任务
    Trigger_PendSV;
}

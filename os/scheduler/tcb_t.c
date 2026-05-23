#include "tcb_t.h"
#include <stdio.h>
#include "thread_scheduler.h"
#include "../common/linear_pool.h"

static void tcb_t_os_sleep(Tcb_t* self, uint32_t ms);

// 析构函数声明
static void tcb_t_destroy(Tcb_t* self);
static void tcb_t_stack_init(Tcb_t *self, Entry_t *entry);

// TODO: 初始化数据成员
static const Tcb_tFun tcb_t_fun = {
    .destroy = tcb_t_destroy,
	.os_sleep = tcb_t_os_sleep,
};
// 构造函数实现
Tcb_t* tcb_t_create(const char *name, Entry_t *entry, mpu_region_size_t stack_size) {
    if (stack_size > DEFAULT_STACK_SIZE) {
        stack_size = DEFAULT_STACK_SIZE;
    }
    Tcb_t* obj = (Tcb_t*)os_ccm_malloc(sizeof(Tcb_t) + (1 << stack_size));
    if (obj) {
        memset(obj, 0, sizeof(Tcb_t) + (1 << stack_size));
        tcb_t_init(obj, name, entry, stack_size);
    }
    return obj;
}

void tcb_t_init(Tcb_t* self, const char *name, Entry_t *entry, mpu_region_size_t stack_size) {
    self->fun = &(tcb_t_fun);
    // TODO: 初始化数据成员
    self->priority = entry->priority;
    self->original_priority = self->priority;
    self->parent = entry->parent;
    self->semaphore = semaphore_create(0);
    GET_NODE(self)->next = NULL;
    self->name = name;
    self->stack_left = 0;
    //self->need_print = false;
    self->stack_size = stack_size;
    tcb_t_stack_init(self, entry);
}

/*
 * 在 Cortex-M4F 中，任意异常（包括 PendSV / SysTick）发生时，硬件自动完成以下压栈（顺序固定）：
    栈顶高地址 ─────────────────────────────
        先前栈帧 （上一次上下文）
        ─────────────────────────────  ← 进入异常前的 SP
        xPSR        (状态寄存器)
        PC          (返回地址)
        LR          (R14)
        R12         (通用寄存器)
        R3          (通用寄存器)
        R2          (通用寄存器)
        R1          (通用寄存器)
        R0          (通用寄存器)
        ─────────────────────────────  ← 当前 SP
        预留 100 字节（FPU 空间，仅当 FPCA=1 时）
        或
        不预留（FPCA=0）
    栈顶低地址 ─────────────────────────────
    同时硬件根据当前 CONTROL.FPCA 设置 LR（EXC_RETURN）：
    FPCA = 0（未用 FPU） → LR = 0xFFFFFFF9（bit4=1）
    FPCA = 1（用过 FPU） → LR = 0xFFFFFFFD（bit4=0）且预留 S0‑S15 + FPSCR 的空间（仅预留空间，不写入数据——这就是 Lazy Stacking 的核心）
 */

static void tcb_t_stack_init(Tcb_t *self, Entry_t *entry) {
// 从栈顶高地址开始
    volatile uint32_t *top = self->stack_ptr + (1 << (self->stack_size - 2));
    for (size_t i = 0; i < (1 << (self->stack_size - 2)); i++) {
        self->stack_ptr[i] = MAGIC_NUM;
    }
// 向下移动，先留出自动压栈区
    top -= 8;
// 填充自动压栈区 地址递减
// CPU已自动将 xPSR, PC, LR, R12, R0-R3 压入当前任务的堆栈
//  xPSR, PC, LR, R12, R0-R3 //R4-R11
    top[0] = (uint32_t)self;      // R0
    top[1] = (uint32_t)entry->arg;// R1
    top[2] = 0;               // R2
    top[3] = 0;               // R3
    top[4] = 0;               // R12
    top[5] = (uint32_t)entry->exit;//0xFFFFFFED;//      // LR (EXC_RETURN)
    top[6] = (uint32_t)entry->entry_fun; // PC
    top[7] = 0x01000000;      // xPSR (Thumb 位)

// 再向下移动，留出 R4-R11, R14区
    top -= 9;                       //R4-R11, R14
    top[8] = EXC_RETURN_THRD_PSP_NF;//R14保存为 EXC_RETURN 为返回的值
    self->sp = top; //此时指向R4-R11区的栈底
    self->state = TCB_STATER_READY;
}
void tcb_t_deinit(Tcb_t* self) {
    // TODO: 数据成员申请资源释放
    self->state = TCB_STATER_TERMINATED;
    if (self->semaphore != NULL) {
        self->semaphore->fun->destroy(self->semaphore);
    }
}

// 析构函数实现
static void tcb_t_destroy(Tcb_t* self) {
    if (self != NULL) {
        tcb_t_deinit(self);
        os_free(self);
    }
}

// os_sleep method
static void tcb_t_os_sleep(Tcb_t* self, uint32_t ms) {
    if (NULL == self) {
        return;
    }
    uint32_t ticks = ms;//ms_to_ticks(ms);   // 毫秒转节拍数
    uint32_t key = arch_irq_lock();
    self->delay_ticks = ticks;
    self->state = TCB_STATER_DELAYED;          // 延时阻塞状态
    // 可选：将任务插入一个专门的延时队列（按唤醒时间排序）
    global_thread_scheduler->delay_list->fun->enqueue(global_thread_scheduler->delay_list, GET_NODE(self));
    // 按 delay_ticks 升序插入
    arch_irq_unlock(key);
    // 触发调度，切换到下一个就绪任务
    Trigger_PendSV;
}


#ifndef TCB_T_H
#define TCB_T_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../common/node.h"
#include "semaphore.h"
#include "../common/util.h"
#include "../driver/hal/hal_fpu.h"
#include "../driver/hal/hal_mpu.h"


#define MAGIC_NUM (0xDEADBEEF)
#define GET_TCB_T(obj) ((Tcb_t *)obj)
// 类声明
typedef struct _Tcb_t Tcb_t;
typedef struct _Tcb_tFun Tcb_tFun;
typedef void (*Tcb_loop)(Tcb_t *self, void *arg);

typedef enum : uint8_t {
    TCB_STATER_READY = 0x10,
    TCB_STATER_RUNNING,
    TCB_STATER_BLOCKED,
    TCB_STATER_DELAYED,
    TCB_STATER_WAITING_MUTEX,
    TCB_STATER_TERMINATED
} Tcb_State;

// 类成员函数结构
struct _Tcb_tFun {
    void (*destroy)(Tcb_t* self);
	void (*os_sleep)(Tcb_t* self, uint32_t ms);

};
/* 单个任务的 CPU 统计信息 */
typedef struct {
    volatile uint64_t    total_run_time;     // 累计运行时间（时间戳单位）
    volatile uint64_t    last_reported_time; // 上次统计时的累计时间
    uint32_t usage_percent;      // 最近一次统计的 CPU 使用率 (%)
} cpu_usage_task_info_t;
/*
 *  EXC_RETURN 关键比特位定义与对比
    位	名称 (ARMv7-M)	ARMv7-M 描述	ARMv8-M (无安全扩展) 描述
    0	保留位	固定为1	固定为0
    2	SPSEL	堆栈指针选择：0 返回后使用MSP，1 返回后使用PSP	含义不变，与 v7-M 相同
    3	Mode	返回模式：0 返回Handler模式，1 返回Thread模式	含义不变，与 v7-M 相同
    4	FType	浮点上下文标志：0 表示栈帧包含FPU寄存器（扩展帧），1 表示标准帧	含义不变，是GDB等调试器识别v8-M栈帧的关键位之一
    5	保留	保留	默认被调用者寄存器堆栈，用于安全扩展
    6	保留	保留	安全或非安全堆栈，用于安全扩展
 *
 * */
typedef enum : uint32_t {
    EXC_RETURN_HAND_MSP_NF = 0xFFFFFFF1,//中断嵌套返回，或裸机中断返回
    EXC_RETURN_THRD_MSP_NF = 0xFFFFFFF9,//裸机系统或RTOS线程使用MSP时，从中断返回
    EXC_RETURN_THRD_PSP_NF = 0xFFFFFFFD,//RTOS任务使用PSP时，从中断返回（最常见场景）
    EXC_RETURN_HAND_MSP_FT = 0xFFFFFFE1,//带FPU保存的Handler模式返回
    EXC_RETURN_THRD_MSP_FT = 0xFFFFFFE9,//带FPU保存，返回使用MSP的Thread模式
    EXC_RETURN_THRD_PSP_FT = 0xFFFFFFED,//带FPU保存，返回使用PSP的Thread模式
} tch_exc_return_t;
typedef enum :uint8_t {
    THREAD_PRIORITY_0 = 0,
    THREAD_PRIORITY_1,
    THREAD_PRIORITY_2,
    THREAD_PRIORITY_3,
    THREAD_PRIORITY_4,
    THREAD_PRIORITY_5,
    THREAD_PRIORITY_6,
    THREAD_PRIORITY_7,
    THREAD_PRIORITY_8,
    THREAD_PRIORITY_9,
    THREAD_PRIORITY_10,
    THREAD_PRIORITY_11,
    THREAD_PRIORITY_12,
    THREAD_PRIORITY_13,
    THREAD_PRIORITY_14,
    THREAD_PRIORITY_15,
    THREAD_PRIORITY_16,
    THREAD_PRIORITY_17,
    THREAD_PRIORITY_18,
    THREAD_PRIORITY_19,
    THREAD_PRIORITY_20,
    THREAD_PRIORITY_21,
    THREAD_PRIORITY_22,
    THREAD_PRIORITY_23,
    THREAD_PRIORITY_24,
    THREAD_PRIORITY_25,
    THREAD_PRIORITY_26,
    THREAD_PRIORITY_27,
    THREAD_PRIORITY_28,
    THREAD_PRIORITY_29,
    THREAD_PRIORITY_30,
    THREAD_PRIORITY_31,
    THREAD_PRIORITY_MAX
} thread_priority_t;
// 类结构
struct _Tcb_t {
    Node base;
    const Tcb_tFun* fun;
    // TODO: 添加数据成员
    void *parent;           //base_task
    const char *name;       //名称
    uint16_t tid;           //线程id
    volatile thread_priority_t priority;      //优先级  0最低 31 最高，优先级低的会被优先级高的打断
    thread_priority_t original_priority; //原始优先级，优先级提升后需要恢复原始优先级
    volatile Tcb_State state;    /* 线程状态 */
    Semaphore *semaphore;   //信号量
    cpu_usage_task_info_t cpu_usage_info;
    volatile size_t stack_left;      //栈剩余大小
    volatile uint32_t delay_ticks;   //剩余等待节拍数
   // bool need_print;
    volatile uint32_t *sp;           //sp指针
    mpu_region_size_t  stack_size;      //栈大小
    uint32_t *stack_ptr;   //栈空间
};

typedef struct {
    thread_priority_t priority;
    mpu_region_size_t stack_size;
    void *parent;
    Tcb_loop loop;
    void *arg;
    void *exit;
} thread_conf_t;
// 构造函数声明
Tcb_t* tcb_t_create(const char *name, const thread_conf_t *conf);
void tcb_t_init(Tcb_t* self, const char *name, const thread_conf_t *conf);

// 析构函数声明
void tcb_t_deinit(Tcb_t* self);

#endif // TCB_T_H
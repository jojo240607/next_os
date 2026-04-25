#ifndef TCB_T_H
#define TCB_T_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../common/node.h"
#include "main.h"
#include "semaphore.h"
#include "../common/util.h"



#define GET_TCB_T(obj) ((Tcb_t *)obj)
// 类声明
typedef struct _Tcb_t Tcb_t;
typedef struct _Tcb_tFun Tcb_tFun;
typedef enum thread_state Tcb_State;
typedef struct _Thread_entry_t Entry_t;
typedef void (*Tcb_entry)(Tcb_t *self, void *arg);

enum thread_state {
    TCB_STATER_READY = 0x10,
    TCB_STATER_RUNNING,
    TCB_STATER_BLOCKED,
    TCB_STATER_DELAYED,
    TCB_STATER_TERMINATED
};

// 类成员函数结构
struct _Tcb_tFun {
    void (*destroy)(Tcb_t* self);
	void (*os_sleep)(Tcb_t* self, uint32_t ms);

};
// 类结构
struct _Tcb_t {
    Node base;
    const Tcb_tFun* fun;
    // TODO: 添加数据成员
    void *parent;//base_task
    const char *name;       //名称
    uint16_t tid;           //线程id
    uint8_t priority;      //优先级
    Tcb_State state;    /* 线程状态 */
    Semaphore *semaphore;   //信号量
    //Tcb_entry entey;        //入口函数
    uint32_t start_time;    //时间片开始时刻计数
    uint32_t run_time;    //时间片结束运行计数
    uint32_t delay_ticks;   //剩余等待节拍数
    uint32_t *sp;           //sp指针
    size_t stack_size;      //栈大小
    uint32_t stack_ptr[];   //栈空间
};

struct _Thread_entry_t {
    uint8_t priority;
    size_t stack_size;
    void *parent;
    Tcb_entry entry_fun;
    void *arg;
    void *exit;
};
// 构造函数声明
Tcb_t* tcb_t_create(const char *name, Entry_t *entry, size_t stack_size);
void tcb_t_init(Tcb_t* self, const char *name, Entry_t *entry, size_t stack_size);

// 析构函数声明
void tcb_t_deinit(Tcb_t* self);

#endif // TCB_T_H
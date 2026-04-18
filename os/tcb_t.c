#include "tcb_t.h"
#include <stdio.h>

// 析构函数声明
static void tcb_t_destroy(Tcb_t* self);

// TODO: 初始化数据成员
static const Tcb_tFun tcb_t_fun = {
    .destroy = tcb_t_destroy,
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

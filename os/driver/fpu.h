#ifndef FPU_H
#define FPU_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "hal/hal_fpu.h"

#define GET_FPU_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(FpuVTable **)obj)
#define GET_FPU(obj) ((Fpu *)obj)

// 派生类声明
typedef struct _Fpu Fpu;
typedef struct _FpuFun FpuFun;
// 类成员函数结构
struct _FpuFun {
    void (*destroy)(Fpu* self);
};
struct _Fpu {
    Device base;  // 基类作为第一个成员
    const FpuFun* fun;
    // TODO: 添加派生类特有的数据成员
    const fpu_config_t *conf;
};

// 构造函数声明
Fpu* fpu_create(const fpu_config_t *conf);
void fpu_init(Fpu* self, const fpu_config_t *conf);

// 析构函数声明
void fpu_deinit(Fpu* self);

#endif // FPU_H
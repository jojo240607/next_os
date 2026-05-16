#ifndef SYSTICK_H
#define SYSTICK_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "../common/sys_time.h"

#define GET_SYSTICK_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(SystickVTable **)obj)
#define GET_SYSTICK(obj) ((Systick *)obj)

// 派生类声明
typedef struct _Systick Systick;
typedef struct _SystickFun SystickFun;

// 类成员函数结构
struct _SystickFun {
    void (*destroy)(Systick* self);
	void (*start)(Systick* self);

	void (*stop)(Systick* self);

};

/* SysTick 配置描述符 */
typedef struct {
    uint32_t    frequency_hz;   // 内核时钟频率 (HCLK)，用于计算重装载值
    uint32_t    interval_us;    // 中断间隔 (微秒)，仅在启用中断时有效
    bool        one_shot;       // 是否单次触发 (true=单次, false=周期)
    void        (*callback)(void); // 中断回调函数 (若为 NULL 则仅产生中断但不处理)
} systick_config_t;

struct _Systick {
    Device base;  // 基类作为第一个成员
    const SystickFun* fun;
    // TODO: 添加派生类特有的数据成员
    const systick_config_t *conf;
};

// 构造函数声明
Systick* systick_create(const systick_config_t *conf);
void systick_init(Systick* self, const systick_config_t *conf);

// 析构函数声明
void systick_deinit(Systick* self);
bool systick_irq_handler_impl(void *arg);

#endif // SYSTICK_H
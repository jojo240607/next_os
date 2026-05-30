#ifndef CAN_H
#define CAN_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "hal/hal_can.h"
#include "common/pinmux.h"

#define GET_CAN_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(CanVTable **)obj)
#define GET_CAN(obj) ((Can *)obj)

// 派生类声明
typedef struct _Can Can;
typedef struct _CanFun CanFun;
// 类成员函数结构
struct _CanFun {
    void (*destroy)(Can* self);
};


/* 过滤器配置 */
typedef struct {
    uint8_t            bank;          // 过滤器组号 (0..27)
    can_filter_mode_t  mode;          // 屏蔽模式 / 列表模式
    can_filter_scale_t scale;         // 16位 / 32位
    can_fifo_t         fifo;          // 关联的 FIFO
    uint32_t           id_high;       // 高4字节
    uint32_t           id_low;        // 低4字节
    bool               active;        // 是否激活
} can_filter_config_t;

/* 引脚配置 */
typedef struct {
    pin_af can_tx;
    pin_af can_rx;
} can_pins_t;

/* CAN 总配置描述符 */
typedef struct {
    can_id_t           id;
    can_mode_t         mode;
    uint32_t           prescaler;          // 预分频系数 (1..1024)
    uint8_t            sjw;                // 同步跳跃宽度 (1..4)
    uint8_t            bs1;                // 时间段 1 (1..16)
    uint8_t            bs2;                // 时间段 2 (1..8)
    //uint32_t           pclk1_hz;           // APB1 时钟频率 (通常 42MHz)
    bool               auto_bus_off;       // 自动总线恢复
    bool               auto_wakeup;        // 自动唤醒
    bool               no_auto_retrans;    // 禁止自动重传
    can_pins_t         pins;
    uint8_t            it_enable;          // 中断使能组合 (CAN_IT_xxx)
    //can_callback_t     callback;           // 中断回调
    uint8_t            num_filters;        // 过滤器个数
    // 过滤器数组指针
    const can_filter_config_t * const filters[];
} can_config_t;

struct _Can {
    Device base;  // 基类作为第一个成员
    const CanFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Can* can_create(const device_info_t *info);
void can_init(Can* self, const device_info_t *info);

// 析构函数声明
void can_deinit(Can* self);

#endif // CAN_H
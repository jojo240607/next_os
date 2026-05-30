#ifndef WDG_H
#define WDG_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"

#define GET_WDG_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(WdgVTable **)obj)
#define GET_WDG(obj) ((Wdg *)obj)

// 派生类声明
typedef struct _Wdg Wdg;
typedef struct _WdgFun WdgFun;

/* ---------- 看门狗类型 ---------- */
typedef enum : uint8_t {
    WDG_IWDG = 0,
    WDG_WWDG = 1
} wdg_type_t;
/* ---------- IWDG 预分频器 (LSI=32kHz) ---------- */
typedef enum : uint8_t {
    IWDG_PRESCALER_4      = 0x00,   // timeout ≈ 0.125ms × reload
    IWDG_PRESCALER_8      = 0x01,   // timeout ≈ 0.25ms  × reload
    IWDG_PRESCALER_16     = 0x02,   // timeout ≈ 0.5ms   × reload
    IWDG_PRESCALER_32     = 0x03,   // timeout ≈ 1ms     × reload
    IWDG_PRESCALER_64     = 0x04,   // timeout ≈ 2ms     × reload
    IWDG_PRESCALER_128    = 0x05,   // timeout ≈ 4ms     × reload
    IWDG_PRESCALER_256    = 0x06,   // timeout ≈ 8ms     × reload
    IWDG_PRESCALER_256_ALT= 0x07    // timeout ≈ 8ms     × reload (别名)
} iwdg_prescaler_t;
/* ---------- WWDG 预分频器 (APB1) ---------- */
typedef enum : uint8_t {
    WWDG_PRESCALER_1   = 0x00,    // PCLK1 / 4096
    WWDG_PRESCALER_2   = 0x01,    // PCLK1 / 8192
    WWDG_PRESCALER_4   = 0x02,    // PCLK1 / 16384
    WWDG_PRESCALER_8   = 0x03     // PCLK1 / 32768
} wwdg_prescaler_t;

/* ---------- IWDG 配置描述符 ---------- */
typedef struct {
    iwdg_prescaler_t prescaler;     // 预分频器
    uint16_t         reload;        // 重装载值 (0..4095, 12位)
} iwdg_config_t;

/* ---------- WWDG 配置描述符 ---------- */
typedef struct {
    wwdg_prescaler_t prescaler;     // 预分频器
    uint8_t          window;        // 窗口值 (0x40..0x7F, 7位)
    uint8_t          counter;       // 初始计数值 (0x40..0x7F)
    bool             enable_ewi;    // 使能提前唤醒中断 (EWI)
    //void (*ewi_callback)(void);     // EWI 中断回调 (可选)
} wwdg_config_t;


// 类成员函数结构
struct _WdgFun {
    void (*destroy)(Wdg* self);
	void (*iwdg_reload)();
    bool (*iwdg_reset_flag)();
	void (*iwdg_clear)();
	uint32_t (*iwdg_timeout)(Wdg* self);

};
struct _Wdg {
    Device base;  // 基类作为第一个成员
    const WdgFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Wdg* wdg_create(const device_info_t *info);
void wdg_init(Wdg* self, const device_info_t *info);

// 析构函数声明
void wdg_deinit(Wdg* self);

#endif // WDG_H
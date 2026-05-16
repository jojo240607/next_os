#ifndef PWM_H
#define PWM_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "timer.h"
#include "common/pinmux.h"

#define GET_PWM_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(PwmVTable **)obj)
#define GET_PWM(obj) ((Pwm *)obj)

// 派生类声明
typedef struct _Pwm Pwm;
typedef struct _PwmFun PwmFun;
// 类成员函数结构
struct _PwmFun {
    void (*destroy)(Pwm* self);
	void (*start)(Pwm* self);
	void (*stop)(Pwm* self);

	void (*set_duty)(Pwm* self, uint8_t channel, uint32_t duty);
	void (*set_period)(Pwm* self, uint32_t autoreload);

};

/* PWM 通道配置 */
typedef struct {
    uint8_t         channel;        // 1~4
    tim_oc_mode_t   mode;           // TIM_OC_MODE_PWM1 或 TIM_OC_MODE_PWM2
    uint32_t        duty;           // 初始占空比 (比较值, 0~autoreload)
    bool            enable_preload; // 使能预装载
    pin_af          pwm_pin;            // 该通道对应的 GPIO 引脚
} pwm_channel_t;

/* PWM 总配置描述符 */
typedef struct {
    tim_id_t            timer_id;       // 定时器编号 (TIM_1 ~ TIM_14)
    tim_timebase_t      timebase;       // 预分频、自动重载等
    uint8_t             num_channels;   // 通道数量
    const pwm_channel_t *const channels[];    // 最多4个通道
} pwm_config_t;

struct _Pwm {
    Device base;  // 基类作为第一个成员
    const PwmFun* fun;
    // TODO: 添加派生类特有的数据成员
    const pwm_config_t *conf;
    Timer* time;
};

// 构造函数声明
Pwm* pwm_create(const pwm_config_t *conf);
void pwm_init(Pwm* self, const pwm_config_t *conf);

// 析构函数声明
void pwm_deinit(Pwm* self);

#endif // PWM_H
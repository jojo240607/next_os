#ifndef RTC_H
#define RTC_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "hal/hal_rtc.h"

#define GET_RTC_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(RtcVTable **)obj)
#define GET_RTC(obj) ((Rtc *)obj)

// 派生类声明
typedef struct _Rtc Rtc;
typedef struct _RtcFun RtcFun;
// 类成员函数结构
struct _RtcFun {
    void (*destroy)(Rtc* self);
};

/* RTC 总配置描述符 */
typedef struct {
    rtc_clk_src_t        clk_src;          // 时钟源
    rtc_hour_format_t    hour_format;      // 12/24 小时制
    uint32_t             async_prediv;     // 异步预分频 (7位, 0..127)
    uint32_t             sync_prediv;      // 同步预分频 (15位, 0..32767)
    rtc_it_t             it_enable;        // RTC_IT_ALARM_A | RTC_IT_ALARM_B | RTC_IT_WAKEUP
    //rtc_callback_t       callback;         // 全局回调
} rtc_config_t;

struct _Rtc {
    Device base;  // 基类作为第一个成员
    const RtcFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Rtc* rtc_create(const device_info_t *info);
void rtc_init(Rtc* self, const device_info_t *info);

// 析构函数声明
void rtc_deinit(Rtc* self);

#endif // RTC_H
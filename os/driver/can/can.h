#ifndef CAN_H
#define CAN_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../hal/hal_can.h"
#include "../common/pinmux.h"
#include "../../scheduler/semaphore.h"

/* CAN 事件 (用于 listener 模式) */
typedef enum : uint8_t {
    CAN_TX_START = 1,
    CAN_TX_DONE,
    CAN_RX_START,
    CAN_RX_DONE,
    CAN_XFER_ERROR,
} can_dev_event_t;

#define GET_CAN_VTABLE(obj) GET_DEVICE_VTABLE(obj)
#define GET_CAN(obj) ((Can *)obj)

typedef struct _Can Can;
typedef struct _CanFun CanFun;

struct _CanFun {
    void (*destroy)(Can* self);
};

/* 过滤器配置 */
typedef struct {
    uint8_t            bank;
    can_filter_mode_t  mode;
    can_filter_scale_t scale;
    can_fifo_t         fifo;
    uint32_t           id_high;
    uint32_t           id_low;
    bool               active;
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
    uint32_t           prescaler;
    uint8_t            sjw;
    uint8_t            bs1;
    uint8_t            bs2;
    bool               auto_bus_off;
    bool               auto_wakeup;
    bool               no_auto_retrans;
    can_pins_t         pins;
    uint8_t            it_enable;
    uint8_t            num_filters;
    const can_filter_config_t * const filters[];
} can_config_t;

struct _Can {
    Device base;
    const CanFun* fun;
    Semaphore     *can_sem;
};

Can* can_create(const device_info_t *info);
void can_init(Can* self, const device_info_t *info);
void can_deinit(Can* self);

#endif // CAN_H

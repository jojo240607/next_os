/**
 * CAN 驱动 — 公共部分 (构造、硬件初始化)
 */
#include "can.h"
#include "../../common/linear_pool.h"
#include "../../log/log.h"
#include "../common/rcc.h"

extern void can_it_dev_init(Device*);
extern void can_it_ioctl(Device*, ioctl_cmd_t, void*);
extern void can_it_write(Device*, const void*, size_t);
extern size_t can_it_read(Device*, void*, size_t);

static void can_hw_init(Can *self);
static void can_destroy(Can* self);
static const CanFun can_fun = { .destroy = can_destroy };

/* ════ 构造 / 析构 ════ */
Can* can_create(const device_info_t *info) {
    Can* obj = (Can*)os_malloc(sizeof(Can));
    if (obj) { memset(obj, 0, sizeof(Can)); can_init(obj, info); }
    return obj;
}

void can_init(Can* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &can_fun;

    GET_DEVICE_VTABLE(self)->dev_init  = can_it_dev_init;
    GET_DEVICE_VTABLE(self)->dev_read  = can_it_read;
    GET_DEVICE_VTABLE(self)->dev_write = can_it_write;
    GET_DEVICE_VTABLE(self)->dev_ioctl = can_it_ioctl;

    self->can_sem = semaphore_create(0);
    can_hw_init(self);
}

void can_deinit(Can* self) { device_deinit(GET_DEVICE(self)); }
static void can_destroy(Can* self) {
    if (self) { can_deinit(self); os_free(self); }
}

/* ════ 内部 listener ════ */
static void default_can_listener(Device *self, uint8_t event, void *arg) {
    Can *can = GET_CAN(self);
    (void)arg;
    switch (event) {
        case CAN_TX_START: case CAN_RX_START:
            can->can_sem->fun->take(can->can_sem); break;
        case CAN_TX_DONE: case CAN_RX_DONE: case CAN_XFER_ERROR:
            can->can_sem->fun->give(can->can_sem); break;
        default: break;
    }
}

/* ════ 公共硬件初始化 ════ */
static void can_hw_init(Can *self) {
    Device *dev = GET_DEVICE(self);
    const can_config_t *conf = dev->info->conf;
    if (!conf || conf->id >= CAN_MAX) return;

    xCAN_TypeDef *can = CANx[conf->id];

    if (conf->id == CAN_1)
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_CAN1EN);
    else
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_CAN2EN);

    pin_config_t pins[2] = {
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP,
         .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE,
         .af = conf->pins.can_tx },
        {.mode = PIN_MODE_AF, .otype = PIN_OTYPE_PP,
         .ospeed = PIN_OSPEED_HIGH, .pupd = PIN_PUPD_NONE,
         .af = conf->pins.can_rx }
    };
    if (pinmux_request_group(pins, 2) != PINMUX_SUCCESS) return;

    hal_can_enter_init_mode(can);
    hal_can_config_mcr(can, conf->auto_bus_off, conf->auto_wakeup, conf->no_auto_retrans);
    hal_can_config_btr(can, conf->mode, conf->sjw, conf->bs1, conf->bs2, conf->prescaler);

    if (conf->num_filters > 0 && *conf->filters) {
        hal_can_filter_init_enter(can);
        for (int i = 0; i < conf->num_filters; i++) {
            const can_filter_config_t *f = conf->filters[i];
            hal_can_config_filter_bank(can, f->bank, f->mode, f->scale,
                                       f->fifo, f->active, f->id_high, f->id_low);
        }
        hal_can_filter_init_exit(can);
    }
    hal_can_exit_init_mode(can);

    dev->fun->register_listener(dev, default_can_listener);
    LOG_DEBUG("can", "can%d hw init ok", conf->id);
}

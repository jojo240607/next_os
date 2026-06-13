/**
 * wdg.c — 看门狗驱动（上层封装）
 * 所有硬件寄存器操作统一通过 hal_wdg_* 函数。
 */
#include "wdg.h"
#include "../common/linear_pool.h"
#include "hal/hal_wdg.h"

dev_ioctl_override(wdg_dev_ioctl_impl);
dev_init_override(wdg_dev_init_impl);

static void wdg_iwdg_reload(void);
static bool wdg_iwdg_reset_flag(void);
static void wdg_iwdg_clear(void);
static uint32_t wdg_iwdg_timeout(Wdg* self);

static void wdg_destroy(Wdg* self);

static const WdgFun wdg_fun = {
    .destroy        = wdg_destroy,
    .iwdg_reload    = wdg_iwdg_reload,
    .iwdg_reset_flag = wdg_iwdg_reset_flag,
    .iwdg_clear     = wdg_iwdg_clear,
    .iwdg_timeout   = wdg_iwdg_timeout,
};

Wdg* wdg_create(const device_info_t *info) {
    Wdg* obj = (Wdg*)os_malloc(sizeof(Wdg));
    if (obj) {
        memset(obj, 0, sizeof(Wdg));
        wdg_init(obj, info);
    }
    return obj;
}

void wdg_init(Wdg* self, const device_info_t *info) {
    device_init(&self->base, info);
    self->fun = &(wdg_fun);
    def_dev_init(self)  = wdg_dev_init_impl;
    def_dev_ioctl(self) = wdg_dev_ioctl_impl;

    if (hal_iwdg_is_reset_flag()) {
        hal_iwdg_clear_reset_flag();
    }
}

void wdg_deinit(Wdg* self) {
    device_deinit(GET_DEVICE(self));
}

static void wdg_destroy(Wdg* self) {
    if (self) { wdg_deinit(self); os_free(self); }
}

/* ── dev_init ── */
dev_init_override(wdg_dev_init_impl) {
    const iwdg_config_t *conf = self->info->conf;
    if (!conf || conf->reload > 0xFFF) return;

    hal_iwdg_unlock();
    hal_iwdg_set_prescaler(conf->prescaler);
    hal_iwdg_set_reload(conf->reload);
    hal_iwdg_wait_ready();
}

/* ── dev_ioctl ── */
dev_ioctl_override(wdg_dev_ioctl_impl) {
    if (cmd == DEVICE_START) {
        hal_iwdg_enable();
        hal_iwdg_reload();
    }
    /* DEVICE_STOP: IWDG 一旦启动无法软件停止 */
}

/* ── IWDG 辅助 ── */
static void wdg_iwdg_reload(void) {
    hal_iwdg_reload();
}

static bool wdg_iwdg_reset_flag(void) {
    return hal_iwdg_is_reset_flag();
}

static void wdg_iwdg_clear(void) {
    hal_iwdg_clear_reset_flag();
}

static uint32_t wdg_iwdg_timeout(Wdg* self) {
    const iwdg_config_t *conf = GET_DEVICE(self)->info->conf;
    if (!conf) return 0;
    uint32_t prescaler_val = 4u << conf->prescaler;
    uint32_t reload = conf->reload + 1;
    uint32_t timeout_us = (prescaler_val * 3125u / 100u) * reload;
    return timeout_us;
}

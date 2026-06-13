#include "fsmc.h"
#include <string.h>
#include "../../common/linear_pool.h"

/* ── 模式文件外部声明 ── */
extern void     fsmc_poll_dev_init(Device *);
extern size_t   fsmc_poll_read(Device *, void *, size_t);
extern void     fsmc_poll_write(Device *, const void *, size_t);
extern void     fsmc_poll_ioctl(Device *, ioctl_cmd_t, void *);
extern void     fsmc_dma_dev_init(Device *);
extern size_t   fsmc_dma_read(Device *, void *, size_t);
extern void     fsmc_dma_write(Device *, const void *, size_t);
extern void     fsmc_dma_ioctl(Device *, ioctl_cmd_t, void *);

static void fsmc_destroy(Fsmc *self);
static void fsmc_hw_init(Fsmc *self);
static const FsmcFun fsmc_fun = { .destroy = fsmc_destroy };

static void default_fsmc_listener(Device *self, uint8_t event, void *arg) {
    Fsmc *fsmc = GET_FSMC(self);
    (void)arg;
    switch (event) {
        case FSMC_XFER_START: fsmc->fsmc_sem->fun->take(fsmc->fsmc_sem); break;
        case FSMC_XFER_DONE:  fsmc->fsmc_sem->fun->give(fsmc->fsmc_sem); break;
        default: break;
    }
}

Fsmc *fsmc_create(const device_info_t *info) {
    Fsmc *obj = (Fsmc *)os_malloc(sizeof(Fsmc));
    if (obj) { memset(obj, 0, sizeof(Fsmc)); fsmc_init(obj, info); }
    return obj;
}

void fsmc_init(Fsmc *self, const device_info_t *info) {
    const fsmc_config_t *conf = info->conf;
    device_init(&self->base, info);
    self->fun = &fsmc_fun;
    self->fsmc_sem = semaphore_create(0);
    self->current_addr = FSMC_BANK1_BASE;
    self->auto_inc = false;            /* 默认 LCD 模式: 地址不自增 */

    if (conf && conf->dma_cfg) {
        GET_DEVICE_VTABLE(self)->dev_init  = fsmc_dma_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = fsmc_dma_read;
        GET_DEVICE_VTABLE(self)->dev_write = fsmc_dma_write;
        GET_DEVICE_VTABLE(self)->dev_ioctl = fsmc_dma_ioctl;
    } else {
        GET_DEVICE_VTABLE(self)->dev_init  = fsmc_poll_dev_init;
        GET_DEVICE_VTABLE(self)->dev_read  = fsmc_poll_read;
        GET_DEVICE_VTABLE(self)->dev_write = fsmc_poll_write;
        GET_DEVICE_VTABLE(self)->dev_ioctl = fsmc_poll_ioctl;
    }

    fsmc_hw_init(self);
}

void fsmc_deinit(Fsmc *self) { device_deinit(&self->base); }

static void fsmc_destroy(Fsmc *self) {
    if (self) { fsmc_deinit(self); os_free(self); }
}

static void fsmc_hw_init(Fsmc *self) {
    const fsmc_config_t *conf = self->base.info->conf;
    if (!conf) return;
    hal_fsmc_clock_enable();

    pin_config_t data_pins[] = {
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data0_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data1_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data2_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data3_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data4_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data5_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data6_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data7_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data8_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data9_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data10_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data11_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data12_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data13_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data14_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.data15_pin },
    };
    if (pinmux_request_group(data_pins, 16) != PINMUX_SUCCESS) return;

    pin_config_t ctrl_pins[] = {
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.ne_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.a_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.wr_pin },
        { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, conf->pins.rd_pin },
    };
    if (pinmux_request_group(ctrl_pins, 4) != PINMUX_SUCCESS) return;

    uint32_t bcr = (1 << 14) | (1 << 12) | (1 << 4) | (1 << 2);
    uint32_t btr = ((conf->address_setup_time & 0x0F) << 0) |
                   ((conf->data_setup_time & 0xFF) << 8) |
                   ((conf->bus_turnaround_time & 0x0F) << 16) |
                   (0x01 << 28);
    hal_fsmc_config_bank1(bcr, btr);

    self->base.fun->register_listener(&self->base, default_fsmc_listener);
}

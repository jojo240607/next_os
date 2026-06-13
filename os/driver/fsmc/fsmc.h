#ifndef FSMC_H
#define FSMC_H
#include <stdint.h>
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../common/gpio.h"
#include "../common/dma.h"
#include "../hal/hal_fsmc.h"
#include "../../scheduler/semaphore.h"

#define GET_FSMC(obj) ((Fsmc *)obj)

/* FSMC 事件 (DMA 模式) */
typedef enum : uint8_t {
    FSMC_XFER_START = 1, FSMC_XFER_DONE,
} fsmc_dev_event_t;

/* FSMC ioctl 命令 */
typedef enum : uint8_t {
    FSMC_IOCTL_SET_ADDR = 1,     /* 设置当前读写地址偏移 */
    FSMC_IOCTL_SET_AUTOINC,      /* 设置地址自增模式: bool*arg, true=自增 */
} fsmc_ioctl_cmd_t;

typedef struct _Fsmc Fsmc;
typedef struct _FsmcFun FsmcFun;
struct _FsmcFun { void (*destroy)(Fsmc *self); };

typedef struct {
    pin_af data0_pin; pin_af data1_pin; pin_af data2_pin; pin_af data3_pin;
    pin_af data4_pin; pin_af data5_pin; pin_af data6_pin; pin_af data7_pin;
    pin_af data8_pin; pin_af data9_pin; pin_af data10_pin; pin_af data11_pin;
    pin_af data12_pin; pin_af data13_pin; pin_af data14_pin; pin_af data15_pin;
    pin_af ne_pin; pin_af a_pin; pin_af wr_pin; pin_af rd_pin;
} fsmc_pins_t;

typedef struct {
    fsmc_pins_t pins;
    uint8_t  address_setup_time;
    uint8_t  data_setup_time;
    uint8_t  bus_turnaround_time;
    dma_stream_config_t *dma_cfg;
} fsmc_config_t;

struct _Fsmc {
    Device base;
    const FsmcFun *fun;
    Semaphore *fsmc_sem;          /* DMA 传输信号量 (listener 使用) */
    uint32_t   current_addr;      /* 当前读写地址 */
    bool       auto_inc;          /* 地址自增: false=LCD模式, true=SRAM/Flash模式 */
};

Fsmc *fsmc_create(const device_info_t *info);
void fsmc_init(Fsmc *self, const device_info_t *info);
void fsmc_deinit(Fsmc *self);

#endif

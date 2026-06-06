#ifndef I2C_H
#define I2C_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/pinmux.h"
#include "common/dma.h"
#include "common/gpio.h"
#include "hal/hal_i2c.h"

#define GET_I2C(obj) ((I2c *)obj)

/* I2C 引脚描述 */
typedef struct {
    pin_af scl_pin;
    pin_af sda_pin;
} i2c_pins_t;

typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} i2c_dma_config_t;

/* I2C 配置描述符 */
typedef struct {
    i2c_id_t        id;
    uint32_t        clock_speed;
    i2c_addr_mode_t addr_mode;
    uint8_t         own_address;
    i2c_pins_t      pins;
    i2c_it_t        it_enable;
    const i2c_dma_config_t *dma_cfg;
} i2c_config_t;

/* ── 中断传输状态 ── */
typedef struct {
    const uint8_t *tx_buf;
    uint8_t       *rx_buf;
    uint16_t       total_len;
    uint16_t       index;
    bool           active;
    bool           direction;        /* 0=TX, 1=RX */
    Semaphore     *i2c_sem;          /* 传输完成信号量 */
} i2c_xfer_state_t;

/*
 * ─── Device VTable 的 override（用户通过 SVC 调用） ───
 * 使用前需通过 ioctl 设置从设备地址：
 *   dev_ioctl(i2c, I2C_IOCTL_SET_ADDR, &slave_addr);
 *   dev_write(i2c, buf, count)  → I2C 主设备发送
 *   dev_read(i2c, buf, count)   → I2C 主设备接收
 */
#define I2C_IOCTL_SET_ADDR    0x60  /* arg = uint8_t* 从设备地址 */
#define I2C_IOCTL_TRANSFER    0x61  /* arg = i2c_transfer_args_t* */

typedef struct {
    uint8_t        slave_addr;
    const uint8_t *tx_buf;
    uint8_t       *rx_buf;
    uint16_t       tx_len;
    uint16_t       rx_len;
} i2c_transfer_args_t;

// 派生类
typedef struct _I2c I2c;
typedef struct _I2cFun I2cFun;

struct _I2cFun {
    void (*destroy)(I2c* self);
};

struct _I2c {
    Device base;
    const I2cFun* fun;
    i2c_xfer_state_t *i2c_xfer;
    uint8_t           slave_addr;     /* 当前从设备地址 */
};

/* 构造函数 */
I2c* i2c_create(const device_info_t *info);
void i2c_init(I2c* self, const device_info_t *info);
void i2c_deinit(I2c* self);

#endif // I2C_H

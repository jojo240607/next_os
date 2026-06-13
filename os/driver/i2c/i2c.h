#ifndef I2C_H
#define I2C_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../common/dma.h"
#include "../common/gpio.h"
#include "../hal/hal_i2c.h"
#include "../../common/linear_pool.h"

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

/*  0：主机写（主机发送数据到从机）
    1：主机读（主机从从机接收数据）
 * */
typedef enum : uint8_t {
    I2C_TX = 0,
    I2C_RX,
} i2c_direction_t;
/* ── 中断传输状态 ── */
typedef struct {
    const uint8_t *tx_buf;      /* TX 数据 */
    uint8_t       *rx_buf;      /* RX 数据 */
    uint16_t       tx_total_len;   /* 当前阶段总长度 */
    uint16_t       rx_total_len;   /* 当前阶段总长度 */
    uint16_t       index;       /* 当前阶段已传输索引 */
    bool           active;
    i2c_direction_t direction;  /* 0=TX, 1=RX */

    /* 组合传输 (写-读): TX 完成后不 STOP, 自动 RESTART 转为 RX */
//    const uint8_t *tx2_buf;     /* TX2 数据 (写寄存器地址) */
//    uint8_t       *rx2_buf;     /* RX2 数据 (读数据) */
//    uint16_t       tx2_len;     /* TX2 长度 */
//    uint16_t       rx2_len;     /* RX2 长度 */
} i2c_xfer_state_t;

typedef enum : uint8_t {
    I2C_TX_START = 1,
    I2C_RX_START,
    I2C_TX_DONE,
    I2C_RX_DONE,
    I2C_TRANS_ERROR,
} i2c_dev_event;
/*
 * ─── Device VTable 的 override（用户通过 SVC 调用） ───
 * 使用前需通过 ioctl 设置从设备地址：
 *   dev_ioctl(i2c, I2C_IOCTL_SET_ADDR, &slave_addr);
 *   dev_write(i2c, buf, count)  → I2C 主设备发送
 *   dev_read(i2c, buf, count)   → I2C 主设备接收
 */
#define I2C_IOCTL_TRANSFER_TX    0x61  /* arg = i2c_transfer_args_t* */
#define I2C_IOCTL_TRANSFER_RX    0x62  /* arg = i2c_transfer_args_t* */
typedef struct {
    uint8_t        slave_addr;/* 从设备地址 */
    const uint8_t *tx_buf;/* 发送数据 */
    uint8_t       *rx_buf;/* 发送长度 */
    uint16_t       tx_len;/* 接收缓冲区 */
    uint16_t       rx_len;/* 接收长度 */
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
    uint8_t           slave_addr;
    Semaphore     *i2c_sem;
};

/* 构造函数 */
I2c* i2c_create(const device_info_t *info);
void i2c_init(I2c* self, const device_info_t *info);
void i2c_deinit(I2c* self);

#endif // I2C_H

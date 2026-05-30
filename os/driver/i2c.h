#ifndef I2C_H
#define I2C_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/pinmux.h"
#include "common/dma.h"
#include "common/gpio.h"
#include "hal/hal_i2c.h"

#define GET_I2C_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(I2cVTable **)obj)
#define GET_I2C(obj) ((I2c *)obj)

// 派生类声明
typedef struct _I2c I2c;
typedef struct _I2cFun I2cFun;
// 类成员函数结构
struct _I2cFun {
    void (*destroy)(I2c* self);
	//void (*transmit)(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
	//void (*receive)(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
	void (*transmit_it)(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
	void (*receive_it)(I2c* self, uint8_t slave_addr, uint8_t *buffer, uint16_t len);
	//void (*transmit_dma)(I2c* self, uint8_t slave_addr, const uint8_t *data, uint16_t len);
};





/* 回调事件类型 */
#define I2C_EVT_TX_COMPLETE    1
#define I2C_EVT_RX_COMPLETE    2
#define I2C_EVT_ERROR          3

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
    uint32_t        clock_speed;    /* 目标频率，如 100000 (标准) 或 400000 (快速) */
    i2c_addr_mode_t addr_mode;      /* I2C_ADDR_7BIT / I2C_ADDR_10BIT */
    uint8_t         own_address;    /* 本机地址 (从模式使用，忽略可设 0) */
    i2c_pins_t      pins;
    /* 中断配置 (可选) */
    i2c_it_t        it_enable;      /* I2C_IT_TXE | I2C_IT_RXNE | I2C_IT_ERR */
    /* DMA 可选配置 (预留扩展) */
    const i2c_dma_config_t *dma_cfg;
} i2c_config_t;
/* 中断传输状态 */
typedef struct {
    const uint8_t *tx_buf;
    uint8_t *rx_buf;
    uint16_t total_len;
    uint16_t index;
    bool active;       /* 传输进行中 */
    bool direction;    /* 0=TX, 1=RX */
    uint8_t slave_addr;
    Semaphore * i2c_tx_sem;
    Semaphore * i2c_rx_sem;
} i2c_xfer_state_t;

struct _I2c {
    Device base;  // 基类作为第一个成员
    const I2cFun* fun;
    // TODO: 添加派生类特有的数据成员
    i2c_xfer_state_t *i2c_xfer;
};

// 构造函数声明
I2c* i2c_create(const device_info_t *info);
void i2c_init(I2c* self, const device_info_t *info);

// 析构函数声明
void i2c_deinit(I2c* self);

#endif // I2C_H
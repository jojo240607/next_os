#ifndef I2S_H
#define I2S_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "hal/hal_i2s.h"
#include "common/pinmux.h"
#include "common/dma.h"

#define GET_I2S_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(I2sVTable **)obj)
#define GET_I2S(obj) ((I2s *)obj)

// 派生类声明
typedef struct _I2s I2s;
typedef struct _I2sFun I2sFun;
// 类成员函数结构
struct _I2sFun {
    void (*destroy)(I2s* self);
};
/* 引脚描述 */
typedef struct {
    pin_af sck_pin;
    pin_af ws_pin;
    pin_af sd_pin;
    pin_af mck_pin;
} i2s_pins_t;
typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} i2s_dma_config_t;
/* I2S 配置描述符 */
typedef struct {
    i2s_id_t            id;                // I2S_2 或 I2S_3
    i2s_mode_t          mode;              // 主/从 + 收/发
    i2s_standard_t      standard;          // Philips/MSB/LSB/PCM
    i2s_data_format_t   data_format;       // 16/24/32位
    i2s_ckpol_t         clock_polarity;    // 时钟极性
    uint32_t            audio_freq;        // 目标采样率 (Hz), 如 44100
    uint32_t            plli2s_n;          // PLLI2SN (192~432)
    uint32_t            plli2s_r;          // PLLI2SR (2~7)
    uint32_t            i2s_div;           // I2SDIV[7:0]
    bool                odd_factor;        // ODD 位
    bool                enable_mck;        // 使能主时钟输出
    i2s_pins_t          pins;
    i2s_it_t            it_enable;         // I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR
    //i2s_callback_t    callback;
    const i2s_dma_config_t *dma_cfg;
} i2s_config_t;



struct _I2s {
    Device base;  // 基类作为第一个成员
    const I2sFun* fun;
    // TODO: 添加派生类特有的数据成员
    const i2s_config_t *conf;
    i2s_xfer_t i2s_xfer;

};

// 构造函数声明
I2s* i2s_create(const i2s_config_t *conf, const dev_pripority_t *priority);
void i2s_init(I2s* self, const i2s_config_t *conf, const dev_pripority_t *priority);

// 析构函数声明
void i2s_deinit(I2s* self);

#endif // I2S_H
#ifndef I2S_H
#define I2S_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../hal/hal_i2s.h"
#include "../common/pinmux.h"
#include "../common/dma.h"
#include "../../scheduler/semaphore.h"

#define GET_I2S(obj) ((I2s *)obj)

typedef struct _I2s I2s;
typedef struct _I2sFun I2sFun;

struct _I2sFun { void (*destroy)(I2s* self); };

/* I2S 事件 */
typedef enum : uint8_t {
    I2S_TX_START = 1, I2S_TX_DONE,
    I2S_RX_START, I2S_RX_DONE,
    I2S_XFER_ERROR,
} i2s_dev_event_t;

/* 引脚描述 */
typedef struct {
    pin_af sck_pin; pin_af ws_pin; pin_af sd_pin; pin_af mck_pin;
} i2s_pins_t;

typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} i2s_dma_config_t;

/* I2S 配置描述符 */
typedef struct {
    i2s_id_t            id;
    i2s_mode_t          mode;
    i2s_standard_t      standard;
    i2s_data_format_t   data_format;
    i2s_ckpol_t         clock_polarity;
    uint32_t            audio_freq;
    uint32_t            plli2s_n;
    uint32_t            plli2s_r;
    uint32_t            i2s_div;
    bool                odd_factor;
    bool                enable_mck;
    i2s_pins_t          pins;
    i2s_it_t            it_enable;
    const i2s_dma_config_t *dma_cfg;
} i2s_config_t;

struct _I2s {
    Device base;
    const I2sFun* fun;
    i2s_xfer_t *i2s_xfer;
    Semaphore     *i2s_tx_sem;
    Semaphore     *i2s_rx_sem;
};

I2s* i2s_create(const device_info_t *info);
void i2s_init(I2s* self, const device_info_t *info);
void i2s_deinit(I2s* self);

#endif // I2S_H

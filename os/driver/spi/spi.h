#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../common/dma.h"
#include "../common/gpio.h"
#include "../hal/hal_spi.h"
#include "../../common/ringbuf.h"
#include "../../scheduler/semaphore.h"

/* SPI 事件 (用于 listener 模式) */
typedef enum : uint8_t {
    SPI_XFER_START = 1,
    SPI_XFER_DONE,
    SPI_XFER_ERROR,
} spi_dev_event_t;

#define GET_SPI(obj) ((Spi *)obj)

/* SPI 引脚描述 */
typedef struct {
    pin_af sck_pin;
    pin_af miso_pin;
    pin_af mosi_pin;
    pin_af nss_pin;
    gpio_t cs_pin;
} spi_pins_t;

typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} spi_dma_config_t;

/* SPI 配置描述符 */
typedef struct {
    spi_id_t            id;
    spi_mode_t          mode;
    spi_frame_t         frame_format;
    spi_br_t            baudrate_div;
    spi_slave_mode_t    master;
    spi_first_bit_t     first_bit;      /* MSB/LSB 优先 */
    spi_nss_mode_t      nss_mode;       /* 软件/硬件 NSS */
    spi_pins_t          pins;
    spi_it_t            it_enable;
    uint16_t            cache_size;
    const spi_dma_config_t *dma_cfg;
} spi_config_t;

/* ---------- 传输状态 ---------- */
typedef struct {
    uint8_t    *buf;        /* 循环接收缓冲区 */
    uint16_t    buf_size;   /* 缓冲区总大小 */
    uint16_t    pos;        /* 实际写入区域大小 */
} spi_cache_t;

typedef struct {
    spi_cache_t *tx_user_buf;
    spi_cache_t *rx_user_buf;
    bool           active;
    //Semaphore     *spi_sem;          /* 传输完成信号量 (IT/DMA 模式) */
    const gpio_t  *cs_pin;          /* 本次传输使用的 CS (IT/DMA 模式) */
} spi_xfer_t;

/*
 * ─── Device VTable 的 override（用户通过 SVC 调用） ───
 * dev_read(spi, buf, count)  → 从 RX cache 取 dev_write 期间收到的数据
 * dev_write(spi, buf, count) → SPI 全双工传输（IT/DMA/阻塞），RX→cache
 * dev_ioctl(spi, cmd, arg)   → 扩展配置
 */
//#define SPI_IOCTL_TRANSFER   0x70  /* arg = spi_transfer_args_t*, 直接指定 tx+rx */

typedef struct {
    const uint8_t *tx_buf;
    uint8_t       *rx_buf;
    uint16_t       len;
    const gpio_t  *cs_pin;     /* CS 引脚 (NULL=使用总线默认 CS) */
} spi_transfer_args_t;

// 派生类
typedef struct _Spi Spi;
typedef struct _SpiFun SpiFun;

struct _SpiFun {
    void (*destroy)(Spi* self);
};

struct _Spi {
    Device base;
    const SpiFun* fun;
    spi_xfer_t *spi_xfer;
    const gpio_t* cs_pin;//软件cs
    Semaphore     *spi_sem;          /* 传输完成信号量 (listener 使用) */
};

/* 构造函数 */
Spi* spi_create(const device_info_t *info);
void spi_init(Spi* self, const device_info_t *info);
void spi_deinit(Spi* self);

#endif // SPI_H

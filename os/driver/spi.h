#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/pinmux.h"
#include "common/dma.h"
#include "common/gpio.h"
#include "hal/hal_spi.h"

#define GET_SPI_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(SpiVTable **)obj)
#define GET_SPI(obj) ((Spi *)obj)

// 派生类声明
typedef struct _Spi Spi;
typedef struct _SpiFun SpiFun;



/* 模式：CPOL + CPHA */
typedef enum : uint8_t {
    SPI_MODE_0 = 0,     // CPOL=0, CPHA=0
    SPI_MODE_1,         // CPOL=0, CPHA=1
    SPI_MODE_2,         // CPOL=1, CPHA=0
    SPI_MODE_3,         // CPOL=1, CPHA=1
} spi_mode_t;

/* 数据帧格式 */
typedef enum :uint8_t {
    SPI_FRAME_8BIT = 0,
    SPI_FRAME_16BIT
} spi_frame_t;

/* 波特率分频 (fPCLK / 2^(br+1)) */
typedef enum : uint8_t {
    SPI_BR_DIV2 = 0,
    SPI_BR_DIV4,
    SPI_BR_DIV8,
    SPI_BR_DIV16,
    SPI_BR_DIV32,
    SPI_BR_DIV64,
    SPI_BR_DIV128,
    SPI_BR_DIV256,
} spi_br_t;

/* 主从模式 */
typedef enum : uint8_t {
    SPI_SLAVE = 0,
    SPI_MASTER = 1,
} spi_slave_mode_t;
typedef enum : uint8_t {
    xSPI_IT_NONE  = (0),
    xSPI_IT_TXE  = (1 << 0),   // 发送缓冲区空中断
    xSPI_IT_RXNE = (1 << 1),   // 接收缓冲区非空中断
    xSPI_IT_ERR  = (1 << 2),   // 错误中断 (CRC/模式/溢出)
} spi_it_t;

/* SPI 引脚描述 */
typedef struct {
    pin_af sck_pin;
    pin_af miso_pin;
    pin_af mosi_pin;
    pin_af nss_pin;    /* 若使用软件 NSS 可忽略 */
} spi_pins_t;

typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} spi_dma_config_t;
/* SPI 配置描述符 */
typedef struct {
    spi_id_t            id;
    spi_mode_t          mode;           /* SPI_MODE_0..3 */
    spi_frame_t         frame_format;   /* SPI_FRAME_8BIT / SPI_FRAME_16BIT */
    spi_br_t            baudrate_div;   /* SPI_BR_DIVx */
    spi_slave_mode_t    master;         /* SPI_MASTER / SPI_SLAVE */
    spi_pins_t          pins;
    /* 中断配置 (可选) */
    //如果同时使用了 SPI 的 TX/RX DMA，DMA 本身的中断可以用来通知传输完成。这时 SPI 外设中断通常不需要打开 TXE/RXNE，而是依赖 DMA 的 TC 中断。
    spi_it_t             it_enable;      // xSPI_IT_TXE | xSPI_IT_RXNE | xSPI_IT_ERR
    /* DMA 可选 */
    const spi_dma_config_t *dma_cfg;   /* 为 NULL 则表示不使用 DMA */
} spi_config_t;

/* ---------- 中断传输状态 ---------- */
typedef struct {
    const uint8_t *tx_buf;
    uint8_t       *rx_buf;
    uint16_t       total_len;
    uint16_t       tx_index;
    uint16_t       rx_index;
    bool           active;
} spi_xfer_t;

// 类成员函数结构
struct _SpiFun {
    void (*destroy)(Spi* self);
	void (*transfer)(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);

	void (*transfer_dma)(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);

	void (*transfer_it)(Spi* self, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);

};
struct _Spi {
    Device base;  // 基类作为第一个成员
    const SpiFun* fun;
    // TODO: 添加派生类特有的数据成员
    const spi_config_t *conf;
    spi_xfer_t spi_xfer;
    Semaphore * spi_tx_sem;
    Semaphore * spi_rx_sem;
};

// 构造函数声明
Spi* spi_create(const spi_config_t *conf);
void spi_init(Spi* self, const spi_config_t *conf);

// 析构函数声明
void spi_deinit(Spi* self);

#endif // SPI_H
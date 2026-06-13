/**
 * sdio.h — SDIO 总线驱动层 (硬件抽象)
 *
 * 职责：SDIO 外设的初始化、原始命令发送、原始数据传输。
 * 不包含任何 SD Card / MMC / SDIO 设备协议。
 * 上层协议驱动（如 sd_card.c）通过 sdio_send_cmd / sdio_xfer_data 操作总线。
 */
#ifndef SDIO_H
#define SDIO_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/gpio.h"
#include "../common/dma.h"
#include "../hal/hal_sdio.h"
#include "../../scheduler/semaphore.h"

#define GET_SDIO(obj) ((Sdio *)obj)

/* ── 总线配置 ── */
typedef enum : uint8_t {
    SDIO_CLOCK_EDGE_RISING = 0, SDIO_CLOCK_EDGE_FALLING = 1
} sdio_clock_edge_t;
typedef enum : uint8_t {
    SDIO_BUS_WIDTH_1 = 0, SDIO_BUS_WIDTH_4 = 1, SDIO_BUS_WIDTH_8 = 2
} sdio_bus_width_t;
typedef enum : uint8_t {
    SDIO_FLOW_DISABLE = 0, SDIO_FLOW_ENABLE = 1
} sdio_hw_flow_t;
typedef enum : uint8_t {
    SDIO_POWERSAVE_DISABLE = 0, SDIO_POWERSAVE_ENABLE = 1
} sdio_power_save_t;

/* ── 引脚 ── */
typedef struct {
    pin_af clk_pin; pin_af cmd_pin; pin_af d0_pin;
    pin_af d1_pin; pin_af d2_pin; pin_af d3_pin;
} sdio_pins_t;

/* ── DMA 配置 ── */
typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} sdio_dma_config_t;
/* ── 中断使能/状态标志 ── */
typedef enum : uint16_t {
    SDIO_IT_CCRCFAIL  = (1 << 0), SDIO_IT_DCRCFAIL  = (1 << 1),
    SDIO_IT_CTIMEOUT  = (1 << 2), SDIO_IT_DTIMEOUT  = (1 << 3),
    SDIO_IT_TXUNDERR  = (1 << 4), SDIO_IT_RXOVERR   = (1 << 5),
    SDIO_IT_CMDREND   = (1 << 6), SDIO_IT_CMDSENT   = (1 << 7),
    SDIO_IT_DATAEND   = (1 << 8), SDIO_IT_DBCKEND   = (1 << 9),
    SDIO_IT_SDIOIT    = (1 << 10),SDIO_IT_TXFIFOHE  = (1 << 11),
    SDIO_IT_RXFIFOHF  = (1 << 12),
} sdio_it_t;
/* ── 总线硬件配置 ── */
typedef struct {
    sdio_clock_edge_t   clock_edge;
    sdio_bus_width_t    bus_width;
    sdio_hw_flow_t      hw_flow_control;
    sdio_power_save_t   power_save;
    uint8_t             clock_div;
    uint32_t            sdio_clk_hz;
    sdio_pins_t         pins;
    sdio_it_t           it_enable;
    const sdio_dma_config_t *dma_cfg;
} sdio_config_t;

/* ── 响应类型 ── */
typedef enum : uint8_t {
    SDIO_RESPONSE_NO          = 0,
    SDIO_RESPONSE_SHORT       = 1,
    SDIO_RESPONSE_SHORT_NO_CRC = 2,
    SDIO_RESPONSE_LONG         = 3
} sdio_response_t;



/* ── 命令描述（总线级，不绑定协议） ── */
typedef struct {
    uint32_t          cmd;          /* 命令索引 */
    uint32_t          arg;          /* 命令参数 */
    sdio_response_t   resp_type;    /* 响应类型 */
    uint32_t          resp[4];      /* 响应数据 */
    int               error;        /* 0=成功, -1=失败 */
} sdio_cmd_t;

/* ── 数据描述（总线级） ── */
typedef struct {
    uint8_t      *buf;
    uint32_t      len;             /* 字节数 (需 4 字节对齐) */
    uint32_t      block_size;      /* 块大小 (用于 DCTRL 配置) */
    bool          dir_to_card;     /* true=写, false=读 */
    int           error;
} sdio_data_t;

/* ── 设备事件 ── */
typedef enum : uint8_t {
    SDIO_CMD_START = 1, SDIO_CMD_DONE,
    SDIO_XFER_START,  SDIO_DATA_DONE, SDIO_XFER_ERROR,
} sdio_dev_event_t;

/* ── 类声明 ── */
typedef struct _Sdio Sdio;
typedef struct _SdioFun SdioFun;

struct _SdioFun { void (*destroy)(Sdio* self); };

struct _Sdio {
    Device          base;
    const SdioFun*  fun;
    Semaphore      *sdio_sem;
    void           *kwork;          /* kwork_t*, 仅 IT/DMA 模式使用 */
};

/* ── 总线级 API ── */
Sdio* sdio_create(const device_info_t *info);
void  sdio_init(Sdio* self, const device_info_t *info);
void  sdio_deinit(Sdio* self);

/* 通过 dev_ioctl(SDIO_CMD_SEND, &cmd) 发送任意命令 */
#define SDIO_CMD_SEND     0x10
/* 通过 dev_ioctl(SDIO_DATA_XFER, &data) 启动数据传输 */
#define SDIO_DATA_XFER     0x11
/* 通过 dev_read / dev_write 进行阻塞式 FIFO 数据搬运 */
/* 通过 hal_sdio_* 直接操作寄存器 */

#endif

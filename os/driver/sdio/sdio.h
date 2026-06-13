#ifndef SDIO_H
#define SDIO_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/gpio.h"
#include "../common/dma.h"
#include "../hal/hal_sdio.h"
#include "../../scheduler/semaphore.h"
#include "../../kernel/kwork.h"
#include "../../common/linear_pool.h"

#define GET_SDIO(obj) ((Sdio *)obj)

#define SDIO_IOCTL_SEND_CMD      0x80  /* arg = sdio_cmd_t*, 发送命令 */
#define SDIO_IOCTL_SET_BLOCK     0x81  /* arg = uint32_t, 设置块地址 */

typedef struct _Sdio Sdio;
typedef struct _SdioFun SdioFun;

typedef enum : uint8_t {
    SDIO_CMD_START = 1, SDIO_CMD_DONE,
    SDIO_XFER_START, SDIO_DATA_DONE, SDIO_XFER_ERROR,
} sdio_dev_event_t;

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
typedef enum : uint8_t {
    SDIO_RESPONSE_NO = 0, SDIO_RESPONSE_SHORT = 1,
    SDIO_RESPONSE_LONG = 3, SDIO_RESPONSE_SHORT_NO_CRC = 2
} sdio_response_t;
typedef enum : uint16_t {
    SDIO_IT_CCRCFAIL  = (1 << 0), SDIO_IT_DCRCFAIL  = (1 << 1),
    SDIO_IT_CTIMEOUT  = (1 << 2), SDIO_IT_DTIMEOUT  = (1 << 3),
    SDIO_IT_TXUNDERR  = (1 << 4), SDIO_IT_RXOVERR   = (1 << 5),
    SDIO_IT_CMDREND   = (1 << 6), SDIO_IT_CMDSENT   = (1 << 7),
    SDIO_IT_DATAEND   = (1 << 8), SDIO_IT_DBCKEND   = (1 << 9),
    SDIO_IT_SDIOIT    = (1 << 10),SDIO_IT_TXFIFOHE  = (1 << 11),
    SDIO_IT_RXFIFOHF  = (1 << 12),
} sdio_it_t;

typedef struct {
    pin_af clk_pin; pin_af cmd_pin; pin_af d0_pin;
    pin_af d1_pin; pin_af d2_pin; pin_af d3_pin;
} sdio_pins_t;

typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} sdio_dma_config_t;

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

typedef struct {
    uint32_t          cmd;
    uint32_t          arg;
    sdio_response_t   resp_type;
    uint32_t          resp[4];
    int               error;
} sdio_cmd_t;

typedef struct {
    uint8_t      *buf;
    uint32_t      len;
    uint32_t      block_size;
    bool          dir_to_card;
    int           error;
} sdio_data_t;

struct _SdioFun { void (*destroy)(Sdio* self); };

struct _Sdio {
    Device base;
    const SdioFun* fun;
    Semaphore     *sdio_sem;         /* 用户层完成信号量 (listener 使用) */
    uint32_t       block_addr;
    uint32_t       block_size;
    kwork_t       *kwork;            /* 工作项指针 (IT/DMA 模式分配, poll 模式 NULL) */
};

Sdio* sdio_create(const device_info_t *info);
void sdio_init(Sdio* self, const device_info_t *info);
void sdio_deinit(Sdio* self);

/* 底层命令发送: 通过 dev_ioctl(SDIO_IOCTL_SEND_CMD, &cmd) 调用 */

#endif
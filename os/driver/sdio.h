#ifndef SDIO_H
#define SDIO_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/gpio.h"
#include "common/dma.h"

#define GET_SDIO_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(SdioVTable **)obj)
#define GET_SDIO(obj) ((Sdio *)obj)

// 派生类声明
typedef struct _Sdio Sdio;
typedef struct _SdioFun SdioFun;

/* ---------- SDIO 时钟边沿 ---------- */
typedef enum : uint8_t {
    SDIO_CLOCK_EDGE_RISING = 0,    /* 上升沿采样 */
    SDIO_CLOCK_EDGE_FALLING = 1    /* 下降沿采样 */
} sdio_clock_edge_t;
/* ---------- SDIO 总线宽度 ---------- */
typedef enum : uint8_t {
    SDIO_BUS_WIDTH_1 = 0,          /* 1-bit 数据总线 */
    SDIO_BUS_WIDTH_4 = 1,          /* 4-bit 数据总线 */
    SDIO_BUS_WIDTH_8 = 2           /* 8-bit 数据总线 (保留) */
} sdio_bus_width_t;

/* ---------- SDIO 硬件流控 ---------- */
typedef enum : uint8_t {
    SDIO_FLOW_DISABLE = 0,
    SDIO_FLOW_ENABLE  = 1
} sdio_hw_flow_t;

/* ---------- SDIO 省电模式 ---------- */
typedef enum : uint8_t {
    SDIO_POWERSAVE_DISABLE = 0,    /* 始终输出时钟 */
    SDIO_POWERSAVE_ENABLE  = 1     /* 仅在总线激活时输出时钟 */
} sdio_power_save_t;

/* ---------- SDIO 响应类型 ---------- */
typedef enum : uint8_t {
    SDIO_RESPONSE_NO      = 0,     /* 无响应 */
    SDIO_RESPONSE_SHORT   = 1,     /* 短响应 (48 bits) */
    SDIO_RESPONSE_LONG    = 3,     /* 长响应 (136 bits) */
    SDIO_RESPONSE_SHORT_NO_CRC = 2 /* 短响应（跳过CRC检查） */
} sdio_response_t;
/* ---------- SDIO 中断事件 ---------- */
typedef enum : uint16_t {
    SDIO_IT_CCRCFAIL  = (1 << 0),   /* 命令CRC失败 */
    SDIO_IT_DCRCFAIL  = (1 << 1),   /* 数据CRC失败 */
    SDIO_IT_CTIMEOUT  = (1 << 2),   /* 命令超时 */
    SDIO_IT_DTIMEOUT  = (1 << 3),   /* 数据超时 */
    SDIO_IT_TXUNDERR  = (1 << 4),   /* 发送下溢 */
    SDIO_IT_RXOVERR   = (1 << 5),   /* 接收上溢 */
    SDIO_IT_CMDREND   = (1 << 6),   /* 命令响应完成 */
    SDIO_IT_CMDSENT   = (1 << 7),   /* 命令发送完成 */
    SDIO_IT_DATAEND   = (1 << 8),   /* 数据结束 */
    SDIO_IT_DBCKEND   = (1 << 9),   /* 数据块结束 */
    SDIO_IT_SDIOIT    = (1 << 10),  /* SDIO中断 */
    SDIO_IT_TXFIFOHE  = (1 << 11),  /* 发送FIFO半空 */
    SDIO_IT_RXFIFOHF  = (1 << 12),  /* 接收FIFO半满 */
} sdio_it_t;

/* ---------- SDIO 引脚描述 ---------- */
typedef struct {
    pin_af clk_pin;     /* SDIO_CK  */
    pin_af cmd_pin;     /* SDIO_CMD */
    pin_af d0_pin;      /* SDIO_D0  */
    pin_af d1_pin;      /* SDIO_D1  */
    pin_af d2_pin;      /* SDIO_D2  */
    pin_af d3_pin;      /* SDIO_D3  */
} sdio_pins_t;
typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} sdio_dma_config_t;
/* ---------- SDIO 配置描述符 ---------- */
typedef struct {
    /* 总线配置 */
    sdio_clock_edge_t   clock_edge;         /* 时钟采样边沿 */
    sdio_bus_width_t    bus_width;          /* 总线宽度 */
    sdio_hw_flow_t      hw_flow_control;    /* 硬件流控 */
    sdio_power_save_t   power_save;         /* 省电模式 */
    uint8_t             clock_div;          /* 分频系数: SDIO_CK = SDIOCLK/(2+CLKDIV) */
    uint32_t            sdio_clk_hz;        /* SDIOCLK频率, 通常48MHz */
    /* 引脚 */
    sdio_pins_t         pins;
    /* 中断使能位(组合值) */
    sdio_it_t            it_enable;          /* SDIO_IT_xxx */
    /* DMA 配置 */
    const sdio_dma_config_t *dma_cfg;
} sdio_config_t;

/* ---------- SDIO 命令结构体 ---------- */
typedef struct {
    uint32_t          cmd;          /* 命令索引 */
    uint32_t          arg;          /* 命令参数 */
    sdio_response_t   resp_type;    /* 响应类型 */
    uint32_t          resp[4];      /* 响应寄存器 (R1..R2) */
    int               error;        /* 0=成功, <0=错误 */
} sdio_cmd_t;

/* ---------- SDIO 数据结构体 ---------- */
typedef struct {
    uint8_t      *buf;              /* 数据缓冲区 */
    uint32_t      len;              /* 数据长度 */
    uint32_t      block_size;       /* 块大小 */
    bool          dir_to_card;      /* true=发送, false=接收 */
    int           error;            /* 0=成功, <0=错误 */
} sdio_data_t;

/* ---------- SD 卡信息 ---------- */
typedef enum : uint8_t {
    SD_CARD_TYPE_UNKNOWN = 0,
    SD_CARD_TYPE_SDHC,
    SD_CARD_TYPE_SDSC,
    SD_CARD_TYPE_MMC
} sd_card_type_t;

// 类成员函数结构
struct _SdioFun {
    void (*destroy)(Sdio* self);
};
struct _Sdio {
    Device base;  // 基类作为第一个成员
    const SdioFun* fun;
    // TODO: 添加派生类特有的数据成员
    const sdio_config_t *conf;
};

// 构造函数声明
Sdio* sdio_create(const sdio_config_t *conf);
void sdio_init(Sdio* self, const sdio_config_t *conf);

// 析构函数声明
void sdio_deinit(Sdio* self);

#endif // SDIO_H
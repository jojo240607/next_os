#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include "device.h"
#include "../hal/hal_dma.h"

#define DMA_ERROR 0
#define DMA_SUCCESS 1



/* 数据流 (0..7) */
typedef uint8_t dma_stream_t;

/* 数据方向 */
typedef enum : uint8_t {
    DMA_DIR_P2M = 0,/* 外设 -> 存储器 */
    DMA_DIR_M2P,    /* 存储器 -> 外设 */
    DMA_DIR_M2M     /* 存储器 -> 存储器 */
} dma_dir;

/* 数据宽度 */
typedef enum : uint8_t {
    DMA_DATA_SIZE_BYTE = 0,
    DMA_DATA_SIZE_HALFWORD,
    DMA_DATA_SIZE_WORD
} dma_data_size;

/* 优先级 */
typedef enum : uint8_t {
    xDMA_PRIORITY_LOW = 0,
    xDMA_PRIORITY_MEDIUM,
    xDMA_PRIORITY_HIGH,
    xDMA_PRIORITY_VERY_HIGH
} dma_priority;


/* 传输模式 */
typedef enum : uint8_t {
    DMA_MODE_NORMAL = 0,/* 单次 */
    DMA_MODE_CIRCULAR   /* 循环 */
} dma_mode;

/* FIFO 模式 */
typedef enum : uint8_t {
    DMA_FIFO_DIRECT = 0, /* 直接模式 */
    DMA_FIFO_ENABLE
} dma_fifo_mode;

typedef enum :uint8_t {
    /* 中断使能选项 */
    xDMA_IT_NONE = 0,
    xDMA_IT_TC = (1 << 0),   /* 传输完成中断 */
    xDMA_IT_HT = (1 << 1),   /* 半传输中断 */
    xDMA_IT_TE = (1 << 2),   /* 传输错误中断 */
} dma_it_t;


/* DMA 流配置描述符 */
typedef struct {
    dma_request_t           dma_request;
    dma_dir                 direction;   /* DMA_DIR_xxx */
    dma_priority            priority;    /* DMA_PRIORITY_xxx */
    dma_data_size           mem_data_size;   /* 存储器数据宽度 */
    dma_data_size           per_data_size;   /* 外设数据宽度 */
    uint8_t                 mem_inc;     /* 存储器地址递增：1 = 使能 */
    uint8_t                 per_inc;     /* 外设地址递增 */
    dma_mode                mode;        /* DMA_MODE_NORMAL / CIRCULAR */
    dma_fifo_mode           fifo_mode;   /* DMA_FIFO_DIRECT / DMA_FIFO_ENABLE */
    dma_it_t                 it_enable;   /* 中断使能 */
} dma_stream_config_t;

/* 函数接口 */
void dma_init(void);
int  dma_stream_request(const dma_stream_config_t *cfg);
int  dma_stream_release(const dma_stream_config_t *cfg);

/* 启动 / 停止传输 */
int  dma_start_transfer(const dma_stream_config_t *cfg,
                        uint32_t src_addr, uint32_t dst_addr, uint16_t count);
int  dma_stop_transfer(const dma_stream_config_t *cfg);

/* 状态查询 */
bool dma_is_busy(const dma_stream_config_t *cfg);
void dma_clear_flag(const dma_stream_config_t *cfg);
nvic_irq_num dma_get_irqnum(const dma_stream_config_t *dma_ctrl);
dma_it_event_t dma_get_it_event(const dma_stream_config_t *cfg);
#endif
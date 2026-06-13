/**
 * sd_card.h — SD Memory Card 协议层
 *
 * 通过 SDIO 总线驱动操作 SD 卡，实现完整的初始化序列和块读写。
 * 不直接访问任何硬件寄存器，全部通过 Sdio 总线的 sdio_cmd_t / sdio_data_t 接口。
 */
#ifndef SD_CARD_H
#define SD_CARD_H
#include <stdint.h>
#include <stdbool.h>
#include "sdio.h"

/* ── SD 卡信息 ── */
typedef struct {
    uint32_t capacity_mb;       /* 容量 (MB) */
    uint32_t block_size;        /* 块大小 (bytes), 通常 512 */
    uint32_t block_count;       /* 总块数 */
    uint8_t  card_type;         /* 0=SDSC, 1=SDHC/SDXC */
    uint8_t  rca;               /* 相对地址 (CMD3 返回) */
    uint8_t  bus_width;         /* 当前总线宽度 (1/4) */
} sd_card_info_t;

/* ── API ── */

/**
 * SD 卡初始化 — 完整上电握手序列
 * sdio: 已初始化的 SDIO 总线设备（poll/it/dma 任意模式）
 * 返回: 0=成功, -1=超时或不支持的卡
 */
int sd_card_init(Sdio *sdio, sd_card_info_t *info);

/**
 * 读取块 (单块模式 CMD17)
 * addr: 逻辑块地址 (0..block_count-1)
 * buf:  数据缓冲区 (需 block_size 大小)
 */
int sd_card_read_block(Sdio *sdio, uint32_t addr, uint8_t *buf);

/**
 * 写入块 (单块模式 CMD24)
 * addr: 逻辑块地址
 * buf:  数据缓冲区
 */
int sd_card_write_block(Sdio *sdio, uint32_t addr, const uint8_t *buf);

#endif

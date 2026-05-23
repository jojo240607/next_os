//
// Created by zhiwei.gong on 2026/5/19.
//

#ifndef STM32F4DISCOVERY_HAL_HASH_H
#define STM32F4DISCOVERY_HAL_HASH_H
#include <stdint.h>
#include <stdbool.h>

/* 哈希算法 */
typedef enum {
    HASH_ALGO_SHA1   = 0,
    HASH_ALGO_SHA224 = 1,
    HASH_ALGO_SHA256 = 2,
    HASH_ALGO_MD5    = 3
} hash_algo_t;

/* HASH 配置描述符 */
typedef struct {
    hash_algo_t algo;
    bool        dma_mode;      // 使用 DMA 传输数据
    void        (*callback)(void);  // 完成中断回调
} hash_config_t;

/* API */
int  hash_init(const hash_config_t *cfg);
void hash_deinit(void);
int  hash_calculate(const uint8_t *data, uint32_t len, uint8_t *digest);
void hash_start(void);
void hash_update(const uint8_t *data, uint32_t len);
void hash_final(uint8_t *digest);
#endif //STM32F4DISCOVERY_HAL_HASH_H

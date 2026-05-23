//
// Created by zhiwei.gong on 2026/5/18.
//

#ifndef STM32F4DISCOVERY_HAL_CRYP_H
#define STM32F4DISCOVERY_HAL_CRYP_H
#include <stdint.h>
#include <stdbool.h>

/* 加密算法 */
typedef enum {
    CRYP_ALGO_ECB_128 = 0,
    CRYP_ALGO_CBC_128 = 1,
    CRYP_ALGO_CTR_128 = 2,
    CRYP_ALGO_ECB_192 = 4,
    CRYP_ALGO_CBC_192 = 5,
    CRYP_ALGO_CTR_192 = 6,
    CRYP_ALGO_ECB_256 = 8,
    CRYP_ALGO_CBC_256 = 9,
    CRYP_ALGO_CTR_256 = 10,
    CRYP_ALGO_DES_ECB = 16,
    CRYP_ALGO_DES_CBC = 17,
    CRYP_ALGO_3DES_ECB = 24,
    CRYP_ALGO_3DES_CBC = 25
} cryp_algo_t;

/* 加解密方向 */
typedef enum {
    CRYP_DIR_ENCRYPT = 0,
    CRYP_DIR_DECRYPT = 1
} cryp_dir_t;

/* CRYP 配置描述符 */
typedef struct {
    cryp_algo_t algo;
    cryp_dir_t  direction;
    const uint8_t *key;         // 密钥 (128/192/256 位)
    const uint8_t *iv;          // 初始化向量 (仅 CBC/CTR 模式)
    uint32_t     data_length;   // 数据长度 (字节)
} cryp_config_t;

/* API */
int  cryp_init(const cryp_config_t *cfg);
void cryp_deinit(void);
int  cryp_process(const uint8_t *input, uint8_t *output, uint32_t len);
void cryp_enable_dma(bool tx, bool rx);
#endif //STM32F4DISCOVERY_HAL_CRYP_H

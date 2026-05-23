//
// Created by zhiwei.gong on 2026/5/18.
//

#ifndef STM32F4DISCOVERY_HAL_CRC_H
#define STM32F4DISCOVERY_HAL_CRC_H
#include <stdint.h>
#include <stdbool.h>

/* CRC 多项式长度 */
typedef enum {
    CRC_POLY_32 = 0,
    CRC_POLY_16 = 1,
    CRC_POLY_8  = 2
} crc_poly_size_t;

/* CRC 配置描述符 */
typedef struct {
    crc_poly_size_t poly_size;   // 多项式位数
    uint32_t        polynomial;  // CRC 多项式 (32/16/8 位)
    uint32_t        init_value;  // 初始值
    bool            reverse_in;  // 输入位序反转
    bool            reverse_out; // 输出位序反转
} crc_config_t;

/* API */
int  crc_init(const crc_config_t *cfg);
void crc_deinit(void);
uint32_t crc_calculate(const uint8_t *data, uint32_t len);
void crc_reset(void);  // 重置 CRC 单元
#endif //STM32F4DISCOVERY_HAL_CRC_H

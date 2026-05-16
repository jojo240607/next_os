//
// Created by Administrator on 2026/5/16/016.
//

#ifndef STM32F4DISCOVERY_FLASH_EEPROM_H
#define STM32F4DISCOVERY_FLASH_EEPROM_H
#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

/* STM32F4 Flash 控制器寄存器 */
#define xFLASH_KEY1      0x45670123UL
#define xFLASH_KEY2      0xCDEF89ABUL
#define xFLASH_SR_BSY    (1 << 16)

/* Flash 存储配置描述符 */
typedef struct {
    uint32_t start_sector;  // 起始扇区号 (如 11, 对应 0x080E0000)
    uint32_t sector_size;   // 扇区大小 (通常 128KB)
    uint8_t  data_size;     // 你打算在扇区内使用的数据长度 (字节)
} flash_eeprom_config_t;

/* API */
int  flash_eeprom_init(const flash_eeprom_config_t *cfg);
int  flash_eeprom_write(const uint8_t *data, uint16_t len);
int  flash_eeprom_read(uint8_t *buffer, uint16_t len);


#endif //STM32F4DISCOVERY_FLASH_EEPROM_H

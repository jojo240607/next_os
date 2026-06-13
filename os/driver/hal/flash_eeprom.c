/**
 * flash_eeprom.c — Flash EEPROM 模拟层
 * 所有寄存器操作通过 hal_flash_* 函数。
 */
#include "flash_eeprom.h"
#include "hal_flash.h"
#include "../../common/util.h"

static uint32_t _start_addr;
static uint32_t _sector_addr;
static uint8_t  _data_size;

int flash_eeprom_init(const flash_eeprom_config_t *cfg) {
    if (!cfg) return -1;
    _start_addr  = 0x08000000 + (cfg->start_sector * cfg->sector_size);
    _sector_addr = _start_addr;
    _data_size   = cfg->data_size;
    return 0;
}

int flash_eeprom_write(const uint8_t *data, uint16_t len) {
    if (len > _data_size) return -1;

    uint32_t primask = __get_PRIMASK();
    DISABLE_IRQ;

    hal_flash_erase_sector(_sector_addr);
    hal_flash_unlock();

    uint32_t *p32 = (uint32_t *)_start_addr;
    uint8_t  *p8  = (uint8_t *)data;
    for (int i = 0; i < len; i += 4) {
        uint32_t word = 0;
        for (int j = 0; j < 4 && (i + j) < len; j++) {
            word |= (uint32_t)p8[i + j] << (8 * j);
        }
        hal_flash_program_word((uint32_t)&p32[i / 4], word);
    }
    hal_flash_lock();

    if (!primask) {
        ENABLE_IRQ;
    }
    return 0;
}

int flash_eeprom_read(uint8_t *buffer, uint16_t len) {
    uint8_t *p_flash = (uint8_t *)_start_addr;
    for (int i = 0; i < len; i++) {
        buffer[i] = p_flash[i];
    }
    return 0;
}

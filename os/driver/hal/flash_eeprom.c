//
// Created by Administrator on 2026/5/16/016.
//

#include "flash_eeprom.h"
#include "../../common/util.h"

static uint32_t _start_addr;   // 起始地址
static uint32_t _sector_addr;  // 所在扇区起始
static uint8_t  _data_size;

/* ── 辅助：解锁/锁定 Flash ── */
static void flash_unlock(void) {
    *(volatile uint32_t *)0x40023C04 = xFLASH_KEY1; // FLASH->KEYR
    *(volatile uint32_t *)0x40023C04 = xFLASH_KEY2;
}
static void flash_lock(void) {
    *(volatile uint32_t *)0x40023C0C |= (1 << 31); // FLASH->CR
}


/* ── 擦除扇区 ── */
static int flash_erase_sector(uint32_t sector_addr) {
    flash_unlock();
    while (*(volatile uint32_t *)0x40023C0C & xFLASH_SR_BSY); // 等待空闲
    *(volatile uint32_t *)0x40023C0C |= (1 << 1); // 设置为扇区擦除
    *(volatile uint32_t *)0x40023C0C |= (sector_addr & 0xFFFFF000) << 3; // 扇区号
    *(volatile uint32_t *)0x40023C0C |= (1 << 16); // 启动擦除
    while (*(volatile uint32_t *)0x40023C0C & xFLASH_SR_BSY);
    flash_lock();
    return 0;
}

/* ── 写一个字 (32bit) ── */
static void flash_program_word(uint32_t addr, uint32_t data) {
    while (*(volatile uint32_t *)0x40023C0C & xFLASH_SR_BSY);
    *(volatile uint32_t *)0x40023C0C |= (1 << 0); // 编程使能
    *(volatile uint32_t *)addr = data;
    while (*(volatile uint32_t *)0x40023C0C & xFLASH_SR_BSY);
}


/* ===================================================================
   初始化
   =================================================================== */
int flash_eeprom_init(const flash_eeprom_config_t *cfg) {
    if (!cfg) return -1;
    _start_addr  = 0x08000000 + (cfg->start_sector * cfg->sector_size);
    _sector_addr = _start_addr;
    _data_size   = cfg->data_size;
    return 0;
}

/* ===================================================================
   写入（断电安全版）
   =================================================================== */
int flash_eeprom_write(const uint8_t *data, uint16_t len) {
    if (len > _data_size) return -1;

    uint32_t primask = __get_PRIMASK();  // 保存全局中断状态
    DISABLE_IRQ;                    // 关键：Flash 写入必须关中断

    // 1. 擦除整个扇区
    flash_erase_sector(_sector_addr);

    // 2. 以 4 字节对齐方式写入数据
    flash_unlock();
    uint32_t *p32 = (uint32_t *)_start_addr;
    uint8_t  *p8  = (uint8_t *)data;
    for (int i = 0; i < len; i += 4) {
        uint32_t word = 0;
        for (int j = 0; j < 4 && (i + j) < len; j++) {
            word |= (uint32_t)p8[i + j] << (8 * j);
        }
        flash_program_word((uint32_t)&p32[i / 4], word);
    }
    flash_lock();

    if (!primask) {
        ENABLE_IRQ;  // 恢复中断
    }
    return 0;
}

/* ===================================================================
   读取（直接读内存地址）
   =================================================================== */
int flash_eeprom_read(uint8_t *buffer, uint16_t len) {
    uint8_t *p_flash = (uint8_t *)_start_addr;
    for (int i = 0; i < len; i++) {
        buffer[i] = p_flash[i];
    }
    return 0;
}
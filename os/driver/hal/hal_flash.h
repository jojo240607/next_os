//
// Created by zhiwei.gong on 2026/5/19.
//

#ifndef STM32F4DISCOVERY_HAL_FLASH_H
#define STM32F4DISCOVERY_HAL_FLASH_H
#include <stdint.h>
#include <stdbool.h>

/* Flash 等待周期 (等待状态数) */
typedef enum {
    FLASH_LATENCY_0WS = 0,   // 0 等待周期，≤ 30 MHz
    FLASH_LATENCY_1WS = 1,
    FLASH_LATENCY_2WS = 2,
    FLASH_LATENCY_3WS = 3,
    FLASH_LATENCY_4WS = 4,
    FLASH_LATENCY_5WS = 5,   // 5 等待周期，≤ 168 MHz (推荐)
    FLASH_LATENCY_6WS = 6,
    FLASH_LATENCY_7WS = 7
} flash_latency_t;

/* Flash 配置描述符 */
typedef struct {
    flash_latency_t latency;     // 等待周期
    bool            enable_prefetch;   // 使能预取指
    bool            enable_icache;     // 使能指令缓存
    bool            enable_dcache;     // 使能数据缓存
} flash_config_t;

/* ── Flash 寄存器 ── */
typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t OPTCR;
} xFLASH_TypeDef;

#define xFLASH_BASE  0x40023C00UL
#define xFLASH       ((xFLASH_TypeDef *)xFLASH_BASE)

/* KEYR 密钥 */
#define xFLASH_KEY1   0x45670123UL
#define xFLASH_KEY2   0xCDEF89ABUL

/* SR 位 */
#define xFLASH_SR_BSY     (1 << 16)

/* CR 位 */
#define xFLASH_CR_PG      (1 << 0)
#define xFLASH_CR_SER     (1 << 1)
#define xFLASH_CR_STRT    (1 << 16)
#define xFLASH_CR_LOCK    (1 << 31)

/* ========== 原 API ========== */
int  hal_flash_init(const flash_config_t *cfg);
void hal_flash_deinit(void);
int  hal_flash_set_latency(flash_latency_t latency);
void hal_flash_enable_prefetch(bool enable);
void hal_flash_enable_icache(bool enable);
void hal_flash_enable_dcache(bool enable);

/* ========== Flash EEPROM API ========== */
void hal_flash_unlock(void);
void hal_flash_lock(void);
void hal_flash_wait_bsy(void);
void hal_flash_erase_sector(uint32_t sector_addr);
void hal_flash_program_word(uint32_t addr, uint32_t data);

#endif //STM32F4DISCOVERY_HAL_FLASH_H

#ifndef LCD_FSMC_H
#define LCD_FSMC_H
#include <stdint.h>
#include "../common/device.h"
#include "../common/gpio.h"
#include "../../scheduler/semaphore.h"
#include "../fsmc/fsmc.h"

#define GET_LCD(obj) ((LcdFsmc *)obj)

typedef struct _LcdFsmc LcdFsmc;
typedef struct _LcdFsmcFun LcdFsmcFun;
struct _LcdFsmcFun { void (*destroy)(LcdFsmc *self); };

/* LCD over FSMC 配置 (纯 LCD, 无 FSMC 总线字段) */
typedef struct {
    dev_id_t bus_dev_id;             /* FSMC 总线设备 ID */
    uint16_t width;
    uint16_t height;
    uint8_t  rs_addr_line;
    gpio_t   rst_pin;
    gpio_t   bl_pin;
    void     (*init_sequence)(void);
} lcd_fsmc_config_t;

struct _LcdFsmc {
    Device base;
    const LcdFsmcFun *fun;
    Device  *fsmc_dev;
    uint16_t width;
    uint16_t height;
    uint32_t cmd_addr;
    uint32_t data_addr;
};

LcdFsmc *lcd_fsmc_create(const device_info_t *info);
void lcd_fsmc_init(LcdFsmc *self, const device_info_t *info);
void lcd_fsmc_deinit(LcdFsmc *self);

void lcd_set_window(LcdFsmc *self, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void lcd_draw_pixel(LcdFsmc *self, uint16_t x, uint16_t y, uint16_t color);
void lcd_fill_screen(LcdFsmc *self, uint16_t color);
int  lcd_fill_screen_dma(LcdFsmc *self, uint16_t *buffer, uint32_t len);

#endif

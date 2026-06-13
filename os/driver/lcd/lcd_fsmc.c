#include "lcd_fsmc.h"
#include <string.h>
#include "../../common/linear_pool.h"
#include "../device_manager.h"
#include "../common/pinmux.h"
#include "../hal/hal_fsmc.h"         /* FSMC_BANK1_BASE */

static void lcd_fsmc_destroy(LcdFsmc *self);
static const LcdFsmcFun lcd_fsmc_fun = { .destroy = lcd_fsmc_destroy };

LcdFsmc *lcd_fsmc_create(const device_info_t *info) {
    LcdFsmc *obj = (LcdFsmc *)os_malloc(sizeof(LcdFsmc));
    if (obj) { memset(obj, 0, sizeof(LcdFsmc)); lcd_fsmc_init(obj, info); }
    return obj;
}

void lcd_fsmc_init(LcdFsmc *self, const device_info_t *info) {
    const lcd_fsmc_config_t *conf = info->conf;
    device_init(&self->base, info);
    self->fun = &lcd_fsmc_fun;

    /* 获取 FSMC 总线设备 */
    self->fsmc_dev = gloable_deviceManager->fun->dev_open(gloable_deviceManager, conf->bus_dev_id);
    self->width    = conf->width;
    self->height   = conf->height;
    self->cmd_addr = FSMC_BANK1_BASE;
    self->data_addr = FSMC_BANK1_BASE + (2UL << conf->rs_addr_line);

    /* 复位 + 背光 GPIO 配置 (FSMC 总线不管 rst/bl, LCD 自管) */
    pin_config_t lcd_pins[] = {
        { .port = conf->rst_pin.port, .pin = conf->rst_pin.pin,
          PIN_MODE_OUTPUT, PIN_OTYPE_PP, PIN_OSPEED_LOW, PIN_PUPD_NONE },
        { .port = conf->bl_pin.port, .pin = conf->bl_pin.pin,
          PIN_MODE_OUTPUT, PIN_OTYPE_PP, PIN_OSPEED_LOW, PIN_PUPD_NONE },
    };
    pinmux_request_group(lcd_pins, 2);

    gpio_reset(&conf->rst_pin);
    for (volatile int i = 0; i < 100000; i++);
    gpio_set(&conf->rst_pin);
    for (volatile int i = 0; i < 100000; i++);
    gpio_set(&conf->bl_pin);

    if (conf->init_sequence) conf->init_sequence();
}

void lcd_fsmc_deinit(LcdFsmc *self) { device_deinit(&self->base); }
static void lcd_fsmc_destroy(LcdFsmc *self) {
    if (self) { lcd_fsmc_deinit(self); os_free(self); }
}

/* ── LCD 底层: 通过 FSMC vtable 读写 ── */
static void lcd_send_cmd(LcdFsmc *self, uint8_t cmd) {
    virtual_dev_ioctl(self->fsmc_dev, FSMC_IOCTL_SET_ADDR, (void *)0);
    virtual_dev_write(self->fsmc_dev, &cmd, 1);
}
static void lcd_send_data(LcdFsmc *self, uint16_t data) {
    virtual_dev_ioctl(self->fsmc_dev, FSMC_IOCTL_SET_ADDR, (void *)self->data_addr);
    virtual_dev_write(self->fsmc_dev, &data, 2);
}

/* ════ LCD API ════ */
void lcd_set_window(LcdFsmc *self, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    lcd_send_cmd(self, 0x2A);
    lcd_send_data(self, x >> 8);  lcd_send_data(self, x & 0xFF);
    lcd_send_data(self, (x + w - 1) >> 8); lcd_send_data(self, (x + w - 1) & 0xFF);
    lcd_send_cmd(self, 0x2B);
    lcd_send_data(self, y >> 8);  lcd_send_data(self, y & 0xFF);
    lcd_send_data(self, (y + h - 1) >> 8); lcd_send_data(self, (y + h - 1) & 0xFF);
    lcd_send_cmd(self, 0x2C);
}

void lcd_draw_pixel(LcdFsmc *self, uint16_t x, uint16_t y, uint16_t color) {
    lcd_set_window(self, x, y, 1, 1);
    lcd_send_data(self, color);
}

void lcd_fill_screen(LcdFsmc *self, uint16_t color) {
    lcd_set_window(self, 0, 0, self->width, self->height);
    virtual_dev_ioctl(self->fsmc_dev, FSMC_IOCTL_SET_ADDR, (void *)self->data_addr);
    for (uint32_t i = (uint32_t)self->width * self->height; i > 0; i--)
        virtual_dev_write(self->fsmc_dev, &color, 2);
}

int lcd_fill_screen_dma(LcdFsmc *self, uint16_t *buffer, uint32_t len) {
    lcd_set_window(self, 0, 0, self->width, self->height);
    virtual_dev_ioctl(self->fsmc_dev, FSMC_IOCTL_SET_ADDR, (void *)self->data_addr);
    virtual_dev_write(self->fsmc_dev, buffer, len * 2);
    return 0;
}

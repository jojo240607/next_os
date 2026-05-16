#include "fsmc.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "common/rcc.h"

dev_init_override(fsmc_dev_init_impl);

// 析构函数声明
static void fsmc_destroy(Fsmc* self);

// TODO: 初始化数据成员
static const FsmcFun fsmc_fun = {
    .destroy = fsmc_destroy,
};
// 构造函数实现
Fsmc* fsmc_create(const fsmc_lcd_config_t *conf) {
    Fsmc* obj = (Fsmc*)os_malloc(sizeof(Fsmc));
    if (obj) {
        memset(obj, 0, sizeof(Fsmc));
        fsmc_init(obj, conf);
    }
    return obj;
}

void fsmc_init(Fsmc* self, const fsmc_lcd_config_t *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(fsmc_fun);
    // TODO: 初始化派生类特有成员

	def_dev_init(self) = fsmc_dev_init_impl;
    self->conf = conf;
}

void fsmc_deinit(Fsmc* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void fsmc_destroy(Fsmc* self) {
    if (self != NULL) {
        fsmc_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(fsmc_dev_init_impl) {
    // TODO: add dev_init method
    Fsmc *fsmc = (Fsmc *)self;
    //params 
    
    if (!fsmc->conf) {
        return;
    }

    // 1. 使能 FSMC 时钟 (AHB3 位 0)
    rcc_periph_clock_enable(RCC_BUS_AHB3, xRCC_AHB3ENR_FSMCEN);

    // 2. 配置引脚 (复用 AF12)
    // 数据线 D0~D15
    // 控制信号线 (NE, A0, WR, RD, RST, BL)
    pin_config_t data_pins[] = {
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data0_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data1_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data2_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data3_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data4_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data5_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data6_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data7_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data8_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data9_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data10_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data11_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data12_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data13_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data14_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.data15_pin },
    };


    if (pinmux_request_group(data_pins, 16) != PINMUX_SUCCESS) {
        return ;
    }

    // 控制信号线 (NE, A0, WR, RD, RST, BL)
    pin_config_t ctrl_pins[] = {
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.ne_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.a_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.wr_pin },
            { .mode = PIN_MODE_AF, PIN_OTYPE_PP, PIN_OSPEED_VERY_HIGH, PIN_PUPD_NONE, fsmc->conf->pins.rd_pin },
            { .port = fsmc->conf->pins.rst_pin.port, .pin = fsmc->conf->pins.rst_pin.pin,
              .mode = PIN_MODE_OUTPUT, PIN_OTYPE_PP, PIN_OSPEED_LOW, PIN_PUPD_NONE },
            { .port = fsmc->conf->pins.bl_pin.port, .pin = fsmc->conf->pins.bl_pin.pin,
              .mode = PIN_MODE_OUTPUT, PIN_OTYPE_PP, PIN_OSPEED_LOW, PIN_PUPD_NONE }
    };
    if (pinmux_request_group(ctrl_pins, 6) != PINMUX_SUCCESS) {
        return ;
    }

    // 3. 配置 FSMC Bank1 Region1 控制寄存器
    // 使用扩展模式，允许独立的读/写时序
    xFSMC_Bank1[0].BCR = (1 << 14) | // EXTMOD = 1
                         (1 << 12) | // WREN = 1
                         (1 << 4)  | // MWID = 01 (16位数据总线)
                         (1 << 2);   // MTYP = 01 (SRAM)

    // 4. 配置 FSMC Bank1 Region1 读时序
    xFSMC_Bank1[0].BTR = ((fsmc->conf->address_setup_time & 0x0F) << 0) |
                        ((fsmc->conf->data_setup_time & 0xFF) << 8) |
                        ((fsmc->conf->bus_turnaround_time & 0x0F) << 16) |
                        (0x01 << 28); // ACCMOD = 01 (访问模式A)

    // 5. 执行 LCD 特定初始化序列
    // 先复位
    //GPIOx[fsmc->conf->rst_port]->BSRR = (1 << (fsmc->conf->rst_pin + 16)); // 拉低RST
    gpio_reset(&fsmc->conf->pins.rst_pin);

    for(volatile int i=0; i<100000; i++);
    //GPIOx[fsmc->conf->rst_port]->BSRR = (1 << fsmc->conf->rst_pin); // 拉高RST
    gpio_set(&fsmc->conf->pins.rst_pin);
    for(volatile int i=0; i<100000; i++);

    // 开背光
    //GPIOx[fsmc->conf->bl_port]->BSRR = (1 << fsmc->conf->bl_pin);
    gpio_set(&fsmc->conf->pins.bl_pin);
    if (fsmc->conf->init_sequence) {
        fsmc->conf->init_sequence();
    }

}

void fsmc_lcd_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // 具体实现依赖LCD驱动IC (如ILI9341)
    // 以ILI9341为例, 需要发送CASET (列地址) 和 PASET (页地址) 命令
    fsmc_lcd_write_cmd(0x2A); // CASET
    fsmc_lcd_write_data(x >> 8);
    fsmc_lcd_write_data(x & 0xFF);
    fsmc_lcd_write_data((x + w - 1) >> 8);
    fsmc_lcd_write_data((x + w - 1) & 0xFF);

    fsmc_lcd_write_cmd(0x2B); // PASET
    fsmc_lcd_write_data(y >> 8);
    fsmc_lcd_write_data(y & 0xFF);
    fsmc_lcd_write_data((y + h - 1) >> 8);
    fsmc_lcd_write_data((y + h - 1) & 0xFF);

    fsmc_lcd_write_cmd(0x2C); // 开始写内存
}

void fsmc_lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    fsmc_lcd_set_window(x, y, 1, 1);
    fsmc_lcd_write_data(color >> 8);
    fsmc_lcd_write_data(color & 0xFF);
}

void fsmc_lcd_fill_screen(Fsmc* self, uint16_t color) {
    fsmc_lcd_set_window(0, 0, self->conf->width, self->conf->height);
    for (uint32_t i = 0; i < (uint32_t)self->conf->width * self->conf->height; i++) {
        fsmc_lcd_write_data(color >> 8);
        fsmc_lcd_write_data(color & 0xFF);
    }
}
/*
 *  正是因为FSMC无法主动发起DMA请求，你代码中已经使用了正确的“变通”方法：
    方向 (direction)：必须使用 DMA_DIR_M2M（内存到内存）模式。
    流/通道 (stream/channel)：在M2M模式下，任何未被占用的Stream和Channel组合都可以使用，
    因为此时DMA流并不与某个特定的硬件外设请求信号绑定。你代码中选择的 .channel = 0 和 .stream = 6 组合，
    只要没有其他外设（如TIM1）同时使用它，就完全可以正常工作。
 */
int fsmc_lcd_fill_screen_dma(Fsmc* self, uint16_t *buffer, uint32_t len)
{
    // 1. 设置LCD显示窗口为全屏
    fsmc_lcd_set_window(0, 0, self->conf->width, self->conf->height);

    // 3. 启动DMA传输
    // 源地址 = 帧缓冲区；目标地址 = LCD数据寄存器
    dma_start_transfer(self->conf->dma_cfg,
                       (uint32_t)buffer,           // 源 (地址自增)
                       (uint32_t)LCD_DATA_ADDR,   // 目标 (地址固定)
                       len);
    return 0;
}


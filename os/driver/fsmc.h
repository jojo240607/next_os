#ifndef FSMC_H
#define FSMC_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/pinmux.h"
#include "common/gpio.h"
#include "hal/hal_fsmc.h"
#include "common/dma.h"

#define GET_FSMC_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(FsmcVTable **)obj)
#define GET_FSMC(obj) ((Fsmc *)obj)

// 派生类声明
typedef struct _Fsmc Fsmc;
typedef struct _FsmcFun FsmcFun;
// 类成员函数结构
struct _FsmcFun {
    void (*destroy)(Fsmc* self);
};

typedef struct {
    // FSMC 引脚配置
    pin_af data0_pin;        // 数据线(D0)
    pin_af data1_pin;        // 数据线(D1)
    pin_af data2_pin;        // 数据线(D2)
    pin_af data3_pin;        // 数据线(D3)
    pin_af data4_pin;        // 数据线(D4)
    pin_af data5_pin;        // 数据线(D5)
    pin_af data6_pin;        // 数据线(D6)
    pin_af data7_pin;        // 数据线(D7)
    pin_af data8_pin;        // 数据线(D8)
    pin_af data9_pin;        // 数据线(D9)
    pin_af data10_pin;       // 数据线(D10)
    pin_af data11_pin;       // 数据线(D11)
    pin_af data12_pin;       // 数据线(D12)
    pin_af data13_pin;       // 数据线(D13)
    pin_af data14_pin;       // 数据线(D14)
    pin_af data15_pin;       // 数据线(D15)
    pin_af ne_pin;           // 片选(CS)
    pin_af a_pin;            // 命令/数据选择(RS) 在FSMC驱动TFT-LCD的硬件设计中，RS（数据/命令选择）引脚需要连接到STM32的任意一根FSMC地址线（A0至A25）上
    pin_af wr_pin;           // 写使能(WR)
    pin_af rd_pin;           // 读使能(RD)
    gpio_t rst_pin;          // 复位(RST)
    gpio_t bl_pin;           // 背光(BL)
} fsmc_pins_t;
/* LCD 配置描述符 */
typedef struct {
    // LCD 尺寸
    uint16_t width;
    uint16_t height;
    fsmc_pins_t pins;
    // FSMC 时序参数 (参考LCD数据手册, HCLK=168MHz时)
    uint8_t address_setup_time;   // 地址建立时间(ADDSET)
    uint8_t data_setup_time;      // 数据建立时间(DATAST)
    uint8_t bus_turnaround_time;  // 总线周转时间(BUSTURN)

    // 函数指针：LCD初始化序列 (不同驱动IC需不同实现)
    void (*init_sequence)(void);
    dma_stream_config_t *dma_cfg;
} fsmc_lcd_config_t;

struct _Fsmc {
    Device base;  // 基类作为第一个成员
    const FsmcFun* fun;
    // TODO: 添加派生类特有的数据成员
    const fsmc_lcd_config_t *conf;
};

// 构造函数声明
Fsmc* fsmc_create(const fsmc_lcd_config_t *conf, const dev_pripority_t *priority);
void fsmc_init(Fsmc* self, const fsmc_lcd_config_t *conf, const dev_pripority_t *priority);

// 析构函数声明
void fsmc_deinit(Fsmc* self);

#endif // FSMC_H
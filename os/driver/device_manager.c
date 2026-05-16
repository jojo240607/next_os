#include "device_manager.h"
#include <stdio.h>
#include "usart.h"
#include "systick.h"
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "device_config.h"
#include "exti.h"
#include "adc.h"
#include "spi.h"
#include "wdg.h"
#include "can.h"
#include "pwm.h"
#include "i2s.h"
#include "fpu.h"

static Device* device_manager_dev_open(Device_manager* self, Dev_tag tag);

// 析构函数声明
static void device_manager_destroy(Device_manager* self);

// TODO: 初始化数据成员
static const Device_managerFun device_manager_fun = {
    .destroy = device_manager_destroy,
	.dev_open = device_manager_dev_open,
};

static const Device_list dev_lists[] = {
        {.tag = DEVICE_SYSTICK, .name = "sys_tick", .config = &sys_tick_conf, .device_create = (Device_create) systick_create},
        {.tag = DEVICE_USART4, .name = "usart4", .config = &usart4_conf, .device_create = (Device_create) usart_create},
        {.tag = DEVICE_USART1, .name = "usart1", .config = &usart1_conf, .device_create = (Device_create) usart_create},
        {.tag = DEVICE_TIME2, .name = "timer2", .config = &time2_conf, .device_create = (Device_create) timer_create},
        {.tag = DEVICE_EXTI, .name = "exti", .config = &exti_conf, .device_create = (Device_create) exti_create},
        {.tag = DEVICE_ADC1, .name = "adc1", .config = &adc1_conf, .device_create = (Device_create) adc_create},
        {.tag = DEVICE_SPI1, .name = "spi1", .config = &spi1_conf, .device_create = (Device_create) spi_create},
        {.tag = DEVICE_I2C1, .name = "i2c1", .config = &i2c1_conf, .device_create = (Device_create) i2c_create},
        {.tag = DEVICE_WDG, .name = "wdg", .config = &iwdg_conf, .device_create = (Device_create) wdg_create},
        {.tag = DEVICE_CAN, .name = "can1", .config = &can1_conf, .device_create = (Device_create) can_create},
        {.tag = DEVICE_PWM1, .name = "pwm1", .config = &pwm1_conf, .device_create = (Device_create) pwm_create},
        {.tag = DEVICE_I2S2, .name = "i2s2", .config = &i2s2_conf, .device_create = (Device_create) i2s_create},
        {.tag = DEVICE_FPU, .name = "fpu", .config = &fpu_conf, .device_create = (Device_create) fpu_create},
};
Device_manager *gloable_deviceManager = NULL;
// 构造函数实现
Device_manager* device_manager_create() {
    Device_manager* obj = (Device_manager*)os_malloc(sizeof(Device_manager) + ARRAY_SIZE(dev_lists) * sizeof(Device *));
    if (obj) {
        memset(obj, 0, sizeof(Device_manager) + ARRAY_SIZE(dev_lists) * sizeof(Device *));
        device_manager_init(obj);
    }
    return obj;
}

void device_manager_init(Device_manager* self) {
    LOG_DEBUG("device_manager", "device_manager_init");
    self->fun = &(device_manager_fun);
    // TODO: 初始化数据成员
    self->devicelist = dev_lists;
    self->dev_size =  ARRAY_SIZE(dev_lists);
}

void device_manager_deinit(Device_manager* self) {
    // TODO: 数据成员申请资源释放
    for (uint8_t i = 0; i < self->dev_size; i++) {
        if (self->dev_tab[i]) {
            self->dev_tab[i]->fun->destroy(self->dev_tab[i]);
            self->dev_tab[i] = NULL;
        }
    }
}

// 析构函数实现
static void device_manager_destroy(Device_manager* self) {
    if (self != NULL) {
        device_manager_deinit(self);
        os_free(self);
    }
}

// dev_open method
static Device* device_manager_dev_open(Device_manager* self, Dev_tag tag) {
    if (self == NULL) {
        return NULL;
    }
    if (tag < 0 || tag >= DEVICE_MAX) {
        return NULL;
    }
    if (self->dev_tab[tag] == NULL) {
        self->dev_tab[tag] = (self->devicelist + tag)->device_create((self->devicelist + tag)->config);
    }
    return self->dev_tab[tag];
}


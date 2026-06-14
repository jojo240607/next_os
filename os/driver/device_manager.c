#include "device_manager.h"
#include <stdio.h>
#include "usart/usart.h"
#include "systick.h"
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "device_config.h"
#include "exti.h"
#include "adc/adc.h"
#include "spi/spi.h"
#include "wdg.h"
#include "can/can.h"
#include "pwm.h"
#include "i2s/i2s.h"
#include "fsmc/fsmc.h"
#include "lcd/lcd_fsmc.h"
#include "usb/usb_cdc.h"

static Device* device_manager_dev_open(Device_manager* self, dev_id_t id);

// 析构函数声明
static void device_manager_destroy(Device_manager* self);

// TODO: 初始化数据成员
static const Device_managerFun device_manager_fun = {
    .destroy = device_manager_destroy,
	.dev_open = device_manager_dev_open,
};

static const Device_list dev_lists[] = {
        {.info = &(const device_info_t){.name = "sys_tick",
                .id = DEVICE_SYSTICK,
                .priority =  &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOWEST, .sub_pripority = 0},
                .conf = &sys_tick_conf},
                .device_create = (Device_create) systick_create},

        {.info = &(const device_info_t){.name = "usart1",
                  .id = DEVICE_USART1,
                  .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_NORMAL, .sub_pripority = 0},
                  .conf = &usart1_conf,},
                  .device_create = (Device_create) usart_create,},
        {.info =  &(const device_info_t){.name = "usart4",
                .id = DEVICE_USART4,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &usart4_conf},
                .device_create = (Device_create) usart_create,},
        {.info = &(const device_info_t){.name = "timer2",
                 .id = DEVICE_TIME2,
                 .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                 .conf = &time2_conf,},
         .device_create = (Device_create) timer_create,},
        {.info = &(const device_info_t){.name = "exti",
                .id = DEVICE_EXTI,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_NORMAL, .sub_pripority = 0},
                .conf = &exti_conf,},
                .device_create = (Device_create) exti_create,},
        {.info = &(const device_info_t){.name = "adc1",
                .id = DEVICE_ADC1,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &adc1_conf,},
                .device_create = (Device_create) adc_create,},
        {.info = &(const device_info_t){.name = "spi1",
                .id = DEVICE_SPI1,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &spi1_conf,},
                .device_create = (Device_create) spi_create,},
        {.info = &(const device_info_t){.name = "i2c1",
                .id = DEVICE_I2C1,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &i2c1_conf,},
                .device_create = (Device_create) i2c_create,},
        {.info = &(const device_info_t){.name = "wdg",
                .id = DEVICE_WDG,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &iwdg_conf,},
                .device_create = (Device_create) wdg_create,},
        {.info = &(const device_info_t){.name = "can1",
                .id = DEVICE_CAN,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &can1_conf,},
                .device_create = (Device_create) can_create,},
        {.info = &(const device_info_t){.name = "pwm1",
                .id = DEVICE_PWM1,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &pwm1_conf,},
                .device_create = (Device_create) pwm_create,},
        {.info = &(const device_info_t){.name = "i2s2",
                .id = DEVICE_I2S2,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &i2s2_conf,},
                .device_create = (Device_create) i2s_create,},
        {.info = &(const device_info_t){.name = "icm20948",
                .id = DEVICE_ICM20948,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &icm20948_conf,},
                .device_create = (Device_create) icm20948_create,},
        {.info = &(const device_info_t){.name = "adxl345",
                .id = DEVICE_ADXL345,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &adxl345_conf,},
                .device_create = (Device_create) adxl345_create,},
        {.info = &(const device_info_t){.name = "fsmc", .id = DEVICE_FSMC,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &fsmc_conf},
                .device_create = (Device_create) fsmc_create,},
        {.info = &(const device_info_t){.name = "lcd", .id = DEVICE_FSMC_LCD,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_LOW2, .sub_pripority = 0},
                .conf = &lcd_fsmc_conf},
                .device_create = (Device_create) lcd_fsmc_create,},
        {.info = &(const device_info_t){.name = "usb_cdc",
                .id = DEVICE_USB_CDC,
                .priority = &(const dev_pripority_t){.peer_pripority = IRQ_PREEMPT_PRIORITY_NORMAL, .sub_pripority = 0},
                .conf = &usb_cdc_conf,},
                .device_create = (Device_create) usb_cdc_create,},

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
static Device* device_manager_dev_open(Device_manager* self, dev_id_t id) {
    if (self == NULL) {
        return NULL;
    }
    if (id < 0 || id >= DEVICE_MAX) {
        return NULL;
    }
    Device **newdevice = self->dev_tab + id;
    if (*newdevice == NULL) {
        *newdevice = (self->devicelist + id)->device_create((self->devicelist + id)->info);
        (*newdevice)->vtable->dev_init(self->dev_tab[id]);
    }
    return *newdevice;
}


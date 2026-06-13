#include "time_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "../driver/svc.h"
#include "../driver/device_manager.h"
#include "../device/icm20948.h"
#include "../device/adxl345.h"

task_start_override(time_task_task_start_impl);

task_init_override(time_task_task_init_impl);
task_thread_override(time_task_task_thread_impl);

// 析构函数声明
static void time_task_destroy(Time_task* self);

// TODO: 初始化数据成员
static const Time_taskFun time_task_fun = {
    .destroy = time_task_destroy,
};
// 构造函数实现
Time_task* time_task_create(const task_into_t *info) {
    Time_task* obj = (Time_task*)os_malloc(sizeof(Time_task));
    if (obj) {
        memset(obj, 0, sizeof(Time_task));
        time_task_init(obj, info);
    }
    return obj;
}

void time_task_init(Time_task* self, const task_into_t *info) {
    // 初始化基类部分
    task_init(&self->base, info);
    self->fun = &(time_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = time_task_task_init_impl;
	def_task_thread(self) = time_task_task_thread_impl;
    def_task_start(self) = time_task_task_start_impl;
    self->timer = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_TIME2);
    self->adc = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_ADC1);
    self->spi = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_SPI1);
    self->i2c = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_I2C1);
    self->icm20948 = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_ICM20948);
    self->adxl345  = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_ADXL345);
   // self->wdg = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_WDG);
    self->pwm = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_PWM1);

}


// task_start method
task_start_override(time_task_task_start_impl) {
    // TODO: add task_start method
    Time_task *time_task = (Time_task *)self;
    //params
    time_task->timer->vtable->dev_ioctl(time_task->timer, DEVICE_START, NULL);
    time_task->pwm->vtable->dev_ioctl(time_task->pwm, DEVICE_START, NULL);//start pwm
    //time_task->wdg->vtable->dev_ioctl(time_task->wdg, DEVICE_START, NULL);//watch dog
}

void time_task_deinit(Time_task* self) {
    task_deinit(GET_TASK(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void time_task_destroy(Time_task* self) {
    if (self != NULL) {
        time_task_deinit(self);
        os_free(self);
    }
}

// task_init method
task_init_override(time_task_task_init_impl) {
    // TODO: add task_init method
    Time_task *time_task = (Time_task *)self;
    //params , void *parent
    if (!time_task) {
        return;
    }
    time_task->timer->fun->attach_irq(time_task->timer, self);

}

// task_thread method
task_thread_override(time_task_task_thread_impl) {
    // TODO: add task_thread method
    Time_task *time_task = (Time_task *)self->parent;
    //params , void *arg
    while (true) {
        sem_take_user(self->semaphore);
        uint32_t adc_data = 0;

        time_task->adc->fun->read_user(time_task->adc, &adc_data, 2);
        LOG_DEBUG("time_task", "----- timer on ----- read ad %x", adc_data);
#ifndef USE_CCMRAM
        /* ── ICM20948 读 WHO_AM_I ── */
        uint8_t icm_id;
        time_task->icm20948->fun->ioctl_user(time_task->icm20948,
                                              ICM20948_IOCTL_READ_ID, &icm_id);
        LOG_DEBUG("time_task", "ICM20948 WHO_AM_I: %02x (expect EA)", icm_id);

        /* ── ADXL345 读 DEVID ── */
#if 1
//        time_task->adxl345->fun->ioctl_user(time_task->adxl345,
//                                            ADXL345_IOCTL_INIT, NULL);
        uint8_t adxl_id;
        time_task->adxl345->fun->ioctl_user(time_task->adxl345,
                                             ADXL345_IOCTL_READ_ID, &adxl_id);
        LOG_DEBUG("time_task", "ADXL345 DEVID: %02x (expect E5)", adxl_id);
        adxl345_accel_t adxl345_accel = {0};
        time_task->adxl345->fun->ioctl_user(time_task->adxl345,
                                            ADXL345_IOCTL_READ_ACCEL, &adxl345_accel);
        LOG_DEBUG("time_task", "ADXL345 accel: %d %d %d", adxl345_accel.accel_x, adxl345_accel.accel_y, adxl345_accel.accel_z);
#else
        /* ── ADXL345 单字节逐次读 (验证 Renode 模型是否支持多字节) ── */
        GET_I2C(time_task->i2c)->slave_addr = 0x53;
        uint8_t raw6[6] = {0};
        for (int i = 0; i < 6; i++) {
            uint8_t reg = 0x32 + i;          // 每次用不同地址
            time_task->i2c->fun->write_user(time_task->i2c, &reg, 1);
            time_task->i2c->fun->read_user(time_task->i2c, &raw6[i], 1);
        }
        LOG_DEBUG("time_task", "ADXL345 single-byte x6: %02x %02x %02x %02x %02x %02x",
                  raw6[0], raw6[1], raw6[2], raw6[3], raw6[4], raw6[5]);
#endif
#endif
        //GET_WDG(time_task->wdg)->fun->iwdg_reload();
        //LOG_DEBUG("time_task", "feed watch dog");
    }
}




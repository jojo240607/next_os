#include "time_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"

task_init_override(time_task_task_init_impl);
task_thread_override(time_task_task_thread_impl);

// 析构函数声明
static void time_task_destroy(Time_task* self);

// TODO: 初始化数据成员
static const Time_taskFun time_task_fun = {
    .destroy = time_task_destroy,
};
// 构造函数实现
Time_task* time_task_create() {
    Time_task* obj = (Time_task*)os_malloc(sizeof(Time_task));
    if (obj) {
        memset(obj, 0, sizeof(Time_task));
        time_task_init(obj);
    }
    return obj;
}

void time_task_init(Time_task* self) {
    // 初始化基类部分
    task_init(&self->base);
    self->fun = &(time_task_fun);
    // TODO: 初始化派生类特有成员

	def_task_init(self) = time_task_task_init_impl;
	def_task_thread(self) = time_task_task_thread_impl;
    self->timer = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_TIME2);
    self->adc = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_ADC1);
    self->spi = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_SPI1);
    self->i2c = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_I2C1);
    self->wdg = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_WDG);
    self->pwm = gloable_deviceManager->fun->dev_open(gloable_deviceManager, DEVICE_PWM1);
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
    //params , void *parent
    if (!time_task) {
        return;
    }
    virtual_dev_init(time_task->timer, self->task_tcb->semaphore);
    virtual_dev_init(time_task->adc, NULL);
    virtual_dev_init(time_task->spi, NULL);
    virtual_dev_init(time_task->i2c, NULL);
   // virtual_dev_init(time_task->wdg, NULL);
    virtual_dev_init(time_task->pwm, NULL);
    GET_TIMER(time_task->timer)->fun->start(GET_TIMER(time_task->timer));

    GET_PWM(time_task->pwm)->fun->start(GET_PWM(time_task->pwm));//start pwm
}
// task_thread method
task_thread_override(time_task_task_thread_impl) {
    // TODO: add task_thread method
    Time_task *time_task = (Time_task *)self->parent;
    //params , void *arg
    while (true) {
        self->semaphore->fun->take(self->semaphore);
        uint32_t adc_data = 0;
        uint8_t tx[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
        uint8_t rx[5];
        time_task->adc->vtable->dev_read(time_task->adc, &adc_data, 2);
        LOG_DEBUG("time_task", "----- timer on ----- read ad %x", adc_data);
        GET_SPI(time_task->spi)->fun->transfer_it(GET_SPI(time_task->spi), tx, rx, 5);
        LOG_DEBUG("time_task", "----- timer on ----- read spi %x %x %x %x %x", rx[0], rx[1], rx[2], rx[3], rx[4]);
        uint8_t reg_addr = 0x10;        // 假设设备寄存器地址
        uint8_t write_val = 0xA5;
        uint8_t read_val = 0;

        // 向从机地址 0x50 的寄存器 0x10 写入 0xA5
        GET_I2C(time_task->i2c)->fun->transmit_it(GET_I2C(time_task->i2c), 0x50, &reg_addr, 1);
        GET_I2C(time_task->i2c)->fun->transmit_it(GET_I2C(time_task->i2c), 0x50, &write_val, 1);
        // 从同一设备地址 0x50 读取一个字节
        GET_I2C(time_task->i2c)->fun->transmit_it(GET_I2C(time_task->i2c), 0x50, &reg_addr, 1);// 先发寄存器地址
        GET_I2C(time_task->i2c)->fun->receive_it(GET_I2C(time_task->i2c), 0x50, &read_val, 1);// 再读回
        LOG_DEBUG("time_task", "----- timer on ----- i2c read addr %x %x", reg_addr, read_val);
        GET_WDG(time_task->wdg)->fun->iwdg_reload();
        LOG_DEBUG("time_task", "feed watch dog");
    }
}


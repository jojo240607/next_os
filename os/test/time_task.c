#include "time_task.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "../driver/svc.h"
#include "../driver/device_manager.h"

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

#pragma pack(push, 1)
typedef struct {
    uint8_t cmd;
    uint8_t data;
} icm20948_data_t;
#pragma pack(pop)        // 恢复之前的对齐值

// 向 ICM-20948 的寄存器写入一个字节
void ICM20948_WriteReg(Device *spi, uint8_t reg, uint8_t data) {
    //uint8_t cmd = reg & 0x7F;       // 确保最高位为 0 (写操作)
    icm20948_data_t regdata = {.cmd = reg & 0x7F,
                            .data = data};

   spi->fun->write_user(spi, &regdata, 2);

}

// 从 ICM-20948 的寄存器读取一个字节
// ICM20948 要求命令和响应在**同一 SPI 事务**内完成，
// 不能用 write+read 两次独立事务（响应会在第二次事务中丢失）。
uint8_t ICM20948_ReadReg(Device *spi, uint8_t reg) {
    uint8_t rx[2] = {0};
    icm20948_data_t regdata = {.cmd = reg | 0x80 ,
            .data = 0x00};// 读 WHO_AM_I + dummy

    spi_transfer_args_t args = {.rx_buf = rx,
            .tx_buf = (uint8_t *)&regdata,
            .len = 2};
    spi->fun->ioctl_user(spi, DEVICE_TRANSFER, &args);

 //   uint8_t tx[2] = {reg | 0x80, 0x00};   // 读命令 + dummy
 //
 //   spi_transfer_args_t args = {.tx_buf = tx, .rx_buf = rx, .len = 2};
 //   spi->fun->ioctl_user(spi, DEVICE_TRANSFER, &args);
    return rx[1];   // 响应在第 2 字节
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
        /*
         * ICM20948 SPI 地址格式: {register[6:0], R/W#}
         *   R/W# = 1 → 读, R/W# = 0 → 写
         *   WHO_AM_I (reg 0x00) 读: (0x00 << 1) | 1 = 0x01
         *   响应在第一字节的下一字节 (rx[1])
         */
        uint8_t data = ICM20948_ReadReg(time_task->spi, 0x00);
        LOG_DEBUG("time_task", "ICM20948 WHO_AM_I: data=%02x (expect EA)", data);
        uint8_t reg_addr = 0x10;        // 假设设备寄存器地址
        uint8_t write_val = 0xA5;
        uint8_t read_val = 0;

        /* ── ADXL345 I2C 轮询读 DEVID ── */
        uint8_t adxl_addr = 0x53;
        time_task->i2c->fun->ioctl_user(time_task->i2c,
                                         I2C_IOCTL_SET_ADDR, &adxl_addr);
        uint8_t reg = 0x00;   // DEVID 寄存器
        time_task->i2c->fun->write_user(time_task->i2c, &reg, 1);
        uint8_t devid = 0;
        time_task->i2c->fun->read_user(time_task->i2c, &devid, 1);
        LOG_DEBUG("time_task", "ADXL345 DEVID: %02x (expect E5)", devid);
        //GET_WDG(time_task->wdg)->fun->iwdg_reload();
        //LOG_DEBUG("time_task", "feed watch dog");
    }
}




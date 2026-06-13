/**
 * ICM-20948 9轴IMU 设备驱动 —— Device 子类
 *
 * SPI 协议: bit7 = R/W# (1=读, 0=写)
 * dev_write({reg, data}, 2)  → 发 {reg & 0x7F, data}
 * dev_read(buf, count)       → 从 cur_reg 发 {reg | 0x80, dummy}, rx[1]=数据
 * dev_ioctl(icm, cmd, arg)   → ICM20948_IOCTL_*
 */
#ifndef ICM20948_H
#define ICM20948_H

#include <stdint.h>
#include "../driver/common/device.h"
#include "../driver/spi/spi.h"

/* ── ioctl 命令 ── */
#define ICM20948_IOCTL_READ_ID      0x00   /* arg = uint8_t*, 读 WHO_AM_I */
#define ICM20948_IOCTL_SET_REG      0x01   /* arg = uint8_t*, 设置当前寄存器指针 */
#define ICM20948_IOCTL_READ_ACCEL   0x02   /* arg = int16_t[3], 读三轴加速度 */
#define ICM20948_IOCTL_READ_GYRO    0x03   /* arg = int16_t[3], 读三轴陀螺仪 */

/* ── 寄存器地址 ── */
#define ICM20948_REG_WHO_AM_I      0x00
#define ICM20948_REG_PWR_MGMT_1    0x06
#define ICM20948_REG_ACCEL_XOUT_H  0x2D
#define ICM20948_REG_GYRO_XOUT_H   0x33

/* ── 配置描述符 ── */
typedef struct {
    spi_id_t spi_id;          /* 挂载的 SPI 总线 */
    gpio_t   cs;              /* CS 引脚 (port + pin) */
} icm20948_config_t;


typedef struct _ICM20948 ICM20948;

struct _ICM20948 {
    Device   base;
    Device  *spi_bus;         /* SPI 总线设备句柄 */
    uint8_t  cur_reg;         /* 当前寄存器指针 */
    gpio_t   cs_pin;          /* 本设备的 CS 引脚 */
};

ICM20948* icm20948_create(const device_info_t *info);
void      icm20948_init(ICM20948* self, const device_info_t *info);
void      icm20948_deinit(ICM20948* self);

#endif

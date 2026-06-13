/**
 * ICM-20948 驱动 —— Device 子类实现
 *
 * SPI 协议: bit7=R/W# (1=读, 0=写), bit[6:0]=寄存器地址
 *   dev_write: 发 2 字节 {reg & 0x7F, data}
 *   dev_read:  发 {reg | 0x80, dummy}, 收 {garbage, data}
 */
#include "icm20948.h"
#include <string.h>
#include "../common/linear_pool.h"
#include "../log/log.h"
#include "../driver/device_manager.h"

dev_init_override(icm20948_dev_init);
dev_read_override(icm20948_dev_read);
dev_write_override(icm20948_dev_write);
dev_ioctl_override(icm20948_dev_ioctl);

static void icm20948_destroy(ICM20948* self)
{
    if (self) { icm20948_deinit(self); os_free(self); }
}

/* ── 构造 / 析构 ── */

ICM20948* icm20948_create(const device_info_t *info)
{
    ICM20948* obj = os_malloc(sizeof(ICM20948));
    if (obj) { memset(obj, 0, sizeof(*obj)); icm20948_init(obj, info); }
    return obj;
}

void icm20948_init(ICM20948* self, const device_info_t *info)
{
    device_init(&self->base, info);
    GET_DEVICE_VTABLE(self)->dev_init  = icm20948_dev_init;
    GET_DEVICE_VTABLE(self)->dev_read  = icm20948_dev_read;
    GET_DEVICE_VTABLE(self)->dev_write = icm20948_dev_write;
    GET_DEVICE_VTABLE(self)->dev_ioctl = icm20948_dev_ioctl;
}

void icm20948_deinit(ICM20948* self)
{
    device_deinit(GET_DEVICE(self));
}

/* ── 内部: SPI 单字节收发 ── */

static uint8_t spi_rw(Device *spi, const gpio_t *cs, uint8_t tx_byte)
{
    uint8_t rx;
    spi_transfer_args_t args = {.tx_buf = &tx_byte, .rx_buf = &rx, .len = 1, .cs_pin = cs};
    virtual_dev_ioctl(spi, DEVICE_TRANSFER, &args);
    return rx;
}

/* ── dev_init: 打开 SPI 总线 ── */

dev_init_override(icm20948_dev_init)
{
    ICM20948 *icm = (ICM20948 *)self;
    const icm20948_config_t *conf = self->info->conf;

    icm->spi_bus = gloable_deviceManager->fun->dev_open(
        gloable_deviceManager, DEVICE_SPI1 + conf->spi_id);
    icm->cur_reg = 0;

    /* 设置 CS 默认高（未选中）并配置为输出 */
    if (conf->cs.pin < PIN_MAX) {
        gpio_set(&conf->cs);
    }
    LOG_DEBUG("icm20948", "init ok, spi%d cs=P%c%d",
              conf->spi_id, 'A' + conf->cs.port, conf->cs.pin);
}

/* ── dev_write: 写寄存器 ──
 *   count=1: buf[0]=reg, 仅设指针
 *   count>=2: buf[0]=reg, buf[1]=data, 发起写事务 */

dev_write_override(icm20948_dev_write)
{
    ICM20948 *icm = (ICM20948 *)self;
    const icm20948_config_t *conf = self->info->conf;
    const uint8_t *p = (const uint8_t *)buf;

    if (count >= 2) {
        uint8_t tx[2] = {(uint8_t)(p[0] & 0x7F), p[1]};
        spi_transfer_args_t args = {.tx_buf = tx, .rx_buf = NULL, .len = 2, .cs_pin = &conf->cs};
        virtual_dev_ioctl(icm->spi_bus, DEVICE_TRANSFER, &args);
        icm->cur_reg = p[0] + 1;
    } else if (count == 1) {
        icm->cur_reg = p[0];    /* 只设指针，不发 SPI */
    }
}

/* ── dev_read: 从 cur_reg 连续读 count 字节 ── */

dev_read_override(icm20948_dev_read)
{
    ICM20948 *icm = (ICM20948 *)self;
    const icm20948_config_t *conf = self->info->conf;
    uint8_t *dst = (uint8_t *)buf;

    for (size_t i = 0; i < count; i++) {
        /* 每读一个寄存器需要 2 字节 SPI: 发地址+dummy, rx[1] 是数据 */
        uint8_t tx[2] = {(uint8_t)(icm->cur_reg | 0x80), 0x00};
        uint8_t rx[2];
        spi_transfer_args_t args = {.tx_buf = tx, .rx_buf = rx, .len = 2, .cs_pin = &conf->cs};
        virtual_dev_ioctl(icm->spi_bus, DEVICE_TRANSFER, &args);
        dst[i] = rx[1];
        icm->cur_reg++;
    }
    return 0;
}

/* ── dev_ioctl: 扩展功能 ── */

dev_ioctl_override(icm20948_dev_ioctl)
{
    ICM20948 *icm = (ICM20948 *)self;

    switch (cmd) {
    case ICM20948_IOCTL_READ_ID: {
        icm->cur_reg = ICM20948_REG_WHO_AM_I;
        virtual_dev_read(self, arg, 1);
        break;
    }
    case ICM20948_IOCTL_SET_REG:
        if (arg) icm->cur_reg = *(uint8_t *)arg;
        break;

    case ICM20948_IOCTL_READ_ACCEL: {
        int16_t *out = (int16_t *)arg;
        icm->cur_reg = ICM20948_REG_ACCEL_XOUT_H;
        uint8_t buf[6];
        virtual_dev_read(self, buf, 6);
        out[0] = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
        out[1] = (int16_t)(((uint16_t)buf[2] << 8) | buf[3]);
        out[2] = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]);
        break;
    }
    case ICM20948_IOCTL_READ_GYRO: {
        int16_t *out = (int16_t *)arg;
        icm->cur_reg = ICM20948_REG_GYRO_XOUT_H;
        uint8_t buf[6];
        virtual_dev_read(self, buf, 6);
        out[0] = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
        out[1] = (int16_t)(((uint16_t)buf[2] << 8) | buf[3]);
        out[2] = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]);
        break;
    }
    default: break;
    }
}

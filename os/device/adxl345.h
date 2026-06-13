/**
 * ADXL345 三轴加速度计 设备驱动 —— Device 子类
 *
 * dev_read(adxl, buf, count)   → 从当前寄存器指针读 count 字节
 * dev_write(adxl, buf, count)  → buf[0]=reg, buf[1]=data (写单个寄存器)
 * dev_ioctl(adxl, cmd, arg)    → ADXL345_IOCTL_*
 */
#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>
#include "../driver/common/device.h"
#include "../driver/i2c/i2c.h"
#include "../kernel/kwork.h"

/* ── ioctl 命令 ── */
#define ADXL345_IOCTL_READ_ID      0x00   /* arg = uint8_t*, 读 DEVID */
#define ADXL345_IOCTL_SET_REG      0x01   /* arg = uint8_t*, 设置寄存器指针 */
#define ADXL345_IOCTL_READ_ACCEL   0x02   /* arg = int16_t[3], 读三轴加速度 */
#define ADXL345_IOCTL_INIT         0x03   /* 初始化 (唤醒+开始测量) */

/* ── 配置描述符 ── */
typedef struct {
    i2c_id_t i2c_id;          /* 挂载的 I2C 总线 */
    uint8_t  slave_addr;      /* I2C 从设备地址 (默认 0x53) */
} adxl345_config_t;


typedef struct _ADXL345 ADXL345;
/* ── I2C 传输状态机 (上下文 + 状态函数) ── */

typedef struct {
    i2c_id_t    id;           /* I2C 外设编号 */
    i2c_transfer_args_t *transfer_args;
} i2c_xfer_t;                 /* 局部上下文，栈分配 */

struct _ADXL345 {
    Device   base;
    Device  *i2c_bus;         /* I2C 总线设备句柄 */
    uint8_t  cur_reg;         /* 当前寄存器指针 */
    kwork_t  *work;
    i2c_xfer_t *i2c_xfer;
    Semaphore  *done;         /* 完成信号量 */
};

typedef enum : uint8_t {
    ADXL_READ_ID_START = 1,
    ADXL_READ_ID_DONE,
    ADXL_INIT_START,
    ADXL_INIT_DONE,
    ADXL_READ_ACCEL_START,
    ADXL_READ_ACCEL_DONE,
    ADXL_WRITE_START,
    ADXL_WRITE_DONE,
    ADXL_READ_START,
    ADXL_READ_DONE,
} adxl345_dev_event_t;

typedef enum : uint8_t {
    WORK_STATE_NONE = 0,
    WORK_STATE_INIT_STEP1,
    WORK_STATE_READID_STEP1,
    WORK_STATE_READID_STEP2,
    WORK_STATE_READACCEL_STEP1,
    WORK_STATE_READACCEL_STEP2,
    WORK_STATE_WRITE,
    WORK_STATE_READ,
} adxl345_work_state_t;


typedef struct {
    uint16_t accel_x;
    uint16_t accel_y;
    uint16_t accel_z;
} adxl345_accel_t;


typedef struct {
    uint8_t reg;
    uint8_t *data;
    uint32_t len;
} adxl345_data_t;

ADXL345* adxl345_create(const device_info_t *info);
void     adxl345_init(ADXL345* self, const device_info_t *info);
void     adxl345_deinit(ADXL345* self);

#endif

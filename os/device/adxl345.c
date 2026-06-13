/**
 * ADXL345 加速度计驱动 —— Device 子类实现
 *
 * I2C 传输改用 kwork/kworker + 轮询 HAL 函数:
 *   旧: virtual_dev_ioctl(I2C_IOCTL_TRANSFER) → SVC → sem_take ❌
 *   新: kwork_submit() → kworker 线程 → i2c_transmit/receive ✓
 *       (用户任务 sem_take 等待完成，不在 SVC 中阻塞)
 */
#include "adxl345.h"
#include <string.h>
#include "../common/linear_pool.h"
#include "../scheduler/semaphore.h"
#include "../driver/hal/hal_i2c.h"
#include "../log/log.h"
#include "../driver/device_manager.h"
#include "../kernel/kwork.h"
#include "../kernel/kworker.h"
typedef void (*work_step)(ADXL345* self);



typedef struct {
    adxl345_work_state_t state;
    work_step step;
} adxl_work_t;
static void adxl_init_step1(ADXL345* self);
static void adxl_readid_step1(ADXL345* self);
static void adxl_readid_step2(ADXL345* self);
static void adxl_readaccel_step1(ADXL345* self);
static void adxl_readaccel_step2(ADXL345* self);
static void adxl_write_step(ADXL345* self);
static void adxl_read_step(ADXL345* self);
/*
 *
 *  WORK_STATE_INIT_STEP1,
    WORK_STATE_READID_STEP1,
    WORK_STATE_READID_STEP2,
    WORK_STATE_READACCEL_STEP1,
    WORK_STATE_READACCEL_STEP2,
    WORK_STATE_WRITE,
    WORK_STATE_READ,
 * */
static const adxl_work_t work_steps[] = {
        {WORK_STATE_INIT_STEP1, adxl_init_step1},
        {WORK_STATE_READID_STEP1, adxl_readid_step1},
        {WORK_STATE_READID_STEP2, adxl_readid_step2},
        {WORK_STATE_READACCEL_STEP1, adxl_readaccel_step1},
        {WORK_STATE_READACCEL_STEP2, adxl_readaccel_step2},
        {WORK_STATE_WRITE, adxl_write_step},
        {WORK_STATE_READ, adxl_read_step}
};

static void adxl_init_step1(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->tx_buf && x->transfer_args->tx_len > 0) {
        self->work->state = WORK_STATE_NONE;
        self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER_RX, x->transfer_args);
    }
}

static void adxl_readaccel_step1(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->tx_buf && x->transfer_args->tx_len > 0) {
        self->work->state = WORK_STATE_READACCEL_STEP2;
        self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER_RX, x->transfer_args);
    }
}
static void adxl_readaccel_step2(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->rx_buf && x->transfer_args->rx_len > 0) {
        self->work->state = WORK_STATE_NONE;
        self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER_RX, x->transfer_args);
    }
}

static void adxl_readid_step1(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->tx_buf && x->transfer_args->tx_len > 0) {
        self->work->state = WORK_STATE_NONE;
        self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER_RX, x->transfer_args);
    }
}
static void adxl_readid_step2(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->rx_buf && x->transfer_args->rx_len > 0) {
        self->work->state = WORK_STATE_NONE;
        //self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER, x->transfer_args);
    }
}

static void adxl_write_step(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->tx_buf && x->transfer_args->tx_len > 0) {
        self->work->state = WORK_STATE_NONE;
        self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER_TX, x->transfer_args);
    }
}
static void adxl_read_step(ADXL345* self) {
    i2c_xfer_t * x = self->i2c_xfer;
    if (x->transfer_args->rx_buf && x->transfer_args->rx_len > 0) {
        self->work->state = WORK_STATE_NONE;
        self->i2c_bus->vtable->dev_ioctl(self->i2c_bus, I2C_IOCTL_TRANSFER_RX, x->transfer_args);
    }
}
/**
 * I2C 传输状态机:
 *   state 1 → i2c_transmit (轮询写)
 *   state 2 → i2c_receive  (轮询读)
 *   done    → sem_give + 返回 0
 */
static int i2c_xfer_fn(kwork_t *work) {
    ADXL345 *adxl345 = (ADXL345 *)work->ctx;
    if (work->state != WORK_STATE_NONE) {
        const adxl_work_t *adxl_work = work_steps + work->state - WORK_STATE_INIT_STEP1;
        adxl_work->step(adxl345);
    }
    return 0;
}

dev_init_override(adxl345_dev_init);
dev_read_override(adxl345_dev_read);
dev_write_override(adxl345_dev_write);
dev_ioctl_override(adxl345_dev_ioctl);

static void adxl345_destroy(ADXL345* self)
{
    if (self) {
        adxl345_deinit(self);
        os_free(self);
    }
}

/* ── 构造 / 析构 ── */

ADXL345* adxl345_create(const device_info_t *info)
{
    ADXL345* obj = os_malloc(sizeof(ADXL345));
    if (obj) { memset(obj, 0, sizeof(*obj)); adxl345_init(obj, info); }
    return obj;
}

void adxl345_init(ADXL345* self, const device_info_t *info)
{
    device_init(&self->base, info);
    GET_DEVICE_VTABLE(self)->dev_init  = adxl345_dev_init;
    GET_DEVICE_VTABLE(self)->dev_read  = adxl345_dev_read;
    GET_DEVICE_VTABLE(self)->dev_write = adxl345_dev_write;
    GET_DEVICE_VTABLE(self)->dev_ioctl = adxl345_dev_ioctl;
    self->work = os_malloc(sizeof(kwork_t));
    memset(self->work, 0, sizeof(kwork_t));
    kwork_init(self->work, i2c_xfer_fn, self);
    self->work->state = WORK_STATE_WRITE;
    self->i2c_xfer = os_malloc(sizeof(i2c_xfer_t));
    memset(self->i2c_xfer, 0, sizeof(i2c_xfer_t));

    const adxl345_config_t *conf = (const adxl345_config_t *)info->conf;
    self->i2c_xfer->id = conf->i2c_id;

    self->i2c_xfer->transfer_args = os_malloc(sizeof(i2c_transfer_args_t));
    self->i2c_xfer->transfer_args->slave_addr = conf->slave_addr;
    self->i2c_xfer->transfer_args->tx_buf = NULL;
    self->i2c_xfer->transfer_args->tx_len = 0;
    self->i2c_xfer->transfer_args->rx_buf = NULL;
    self->i2c_xfer->transfer_args->rx_len = 0;
    self->done = semaphore_create(0);

}

void adxl345_deinit(ADXL345* self)
{
    device_deinit(GET_DEVICE(self));
}

static void i2c_listener(Device *self, uint8_t event, void *arg) {
    Device *adxl345 = (Device *)arg;
    switch (event) {
        case I2C_TX_DONE:
            /* 如果只有 TX、无 RX（如写操作）, 直接通知 adxl 完成 */
            if (adxl345->current_event == ADXL_WRITE_START) {
                adxl345->fun->trigger_event(
                    adxl345, ADXL_WRITE_DONE, NULL);
            } else {//if (adxl345->current_event == ADXL_READ_ID_START) {
                /* TX 完成: 唤醒 kworker (驱动下一步) 或直接放行 */
                gloable_kworker->wake->fun->give(gloable_kworker->wake);
            }
            break;
        case I2C_RX_DONE:
            /* RX 完成 → 整次传输结束 */
            GET_DEVICE(adxl345)->fun->trigger_event(
                GET_DEVICE(adxl345), adxl345->current_event + 1, NULL);
            break;
        case I2C_TX_START:
        case I2C_RX_START:
        case I2C_TRANS_ERROR:
        default:
            break;
    }
}

static void default_adxl_listener(Device *self, uint8_t event, void *arg) {
    ADXL345 *adxl345 = (ADXL345 *)self;
    switch (event) {
        case ADXL_READ_ID_START:
        case ADXL_READ_ACCEL_START:
        case ADXL_WRITE_START:
        case ADXL_READ_START:
        case ADXL_INIT_START:
            adxl345->done->fun->take(adxl345->done);
            break;
        case ADXL_READ_ID_DONE:
        case ADXL_READ_ACCEL_DONE:
        case ADXL_WRITE_DONE:
        case ADXL_READ_DONE:
        case ADXL_INIT_DONE:
            adxl345->done->fun->give(adxl345->done);
            break;
        default:
            break;
    }
}
/* ── dev_init: 打开 I2C 总线 ── */

dev_init_override(adxl345_dev_init)
{
    ADXL345 *adxl = (ADXL345 *)self;
    const adxl345_config_t *conf = self->info->conf;

    adxl->i2c_bus = gloable_deviceManager->fun->dev_open(
        gloable_deviceManager, DEVICE_I2C1 + conf->i2c_id);
    adxl->i2c_bus->arg = self;
    GET_I2C(adxl->i2c_bus)->slave_addr = conf->slave_addr;
    adxl->cur_reg = 0;
    LOG_DEBUG("adxl345", "init ok, i2c%d addr=0x%02X", conf->i2c_id, conf->slave_addr);
    adxl->i2c_bus->fun->register_listener(adxl->i2c_bus, i2c_listener);//i2c回调
    self->fun->register_listener(self, default_adxl_listener);//默认adxl驱动回调
}

/* ── dev_write: 写寄存器 ── */
dev_write_override(adxl345_dev_write)
{
    ADXL345 *adxl = (ADXL345 *)self;
    const uint8_t *p = (const uint8_t *)buf;

    if (count >= 2) {
        adxl->i2c_xfer->transfer_args->tx_buf = p;
        adxl->i2c_xfer->transfer_args->tx_len = count;
        adxl->i2c_xfer->transfer_args->rx_buf = NULL;
        adxl->i2c_xfer->transfer_args->rx_len = 0;
        adxl->work->state = WORK_STATE_WRITE;
        adxl->cur_reg = p[0];
        kwork_submit(gloable_kworker, adxl->work);
        self->fun->trigger_event(self, ADXL_WRITE_START, NULL);

    } else if (count == 1) {
        adxl->cur_reg = p[0];
    }
}

/* ── dev_read: 从 cur_reg 读 count 字节 ──
 * Renode ADXL345 不支持多字节连续读, 所以逐字节循环。
 * 每次读取: 写寄存器地址 → RESTART → 读 1 字节。
 */
dev_read_override(adxl345_dev_read)
{
    ADXL345 *adxl = (ADXL345 *)self;
    uint8_t *dst = (uint8_t *)buf;
    for (size_t i = 0; i < count; i++) {
        uint8_t reg = adxl->cur_reg;
        adxl->i2c_xfer->transfer_args->tx_buf = &reg;
        adxl->i2c_xfer->transfer_args->tx_len = 1;
        adxl->i2c_xfer->transfer_args->rx_buf = &dst[i];
        adxl->i2c_xfer->transfer_args->rx_len = 1;
        virtual_dev_ioctl(adxl->i2c_bus, I2C_IOCTL_TRANSFER_RX,
                          adxl->i2c_xfer->transfer_args);
        adxl->cur_reg++;
    }
    return 0;
}
static const uint8_t init_cmd[2] = {0x2D, 0x08};
/* ── dev_ioctl ── */
dev_ioctl_override(adxl345_dev_ioctl)
{
    ADXL345 *adxl = (ADXL345 *)self;
    //const adxl345_config_t *conf = (const adxl345_config_t *)self->info->conf;

    switch (cmd) {
    case ADXL345_IOCTL_READ_ID: {
        adxl->cur_reg = 0x00;  /* DEVID */
        adxl->i2c_xfer->transfer_args->tx_buf = &adxl->cur_reg;
        adxl->i2c_xfer->transfer_args->tx_len = 1;
        adxl->i2c_xfer->transfer_args->rx_buf = arg;
        adxl->i2c_xfer->transfer_args->rx_len = 1;
        virtual_dev_ioctl(adxl->i2c_bus, I2C_IOCTL_TRANSFER_RX, adxl->i2c_xfer->transfer_args);
        break;
    }
    case ADXL345_IOCTL_SET_REG:
        if (arg) {
            adxl->cur_reg = *(uint8_t *)arg;
        }
        break;

    case ADXL345_IOCTL_READ_ACCEL: {
        adxl->cur_reg = 0x32;  /* DATAX0 */
        virtual_dev_read(self, arg, sizeof(adxl345_accel_t));
        break;
    }
    case ADXL345_IOCTL_INIT: {

        adxl->i2c_xfer->transfer_args->tx_buf = init_cmd;
        adxl->i2c_xfer->transfer_args->tx_len = 2;
        adxl->i2c_xfer->transfer_args->rx_buf =  NULL;
        adxl->i2c_xfer->transfer_args->rx_len = 0;
        adxl->work->state = WORK_STATE_INIT_STEP1;
        //kwork_submit(gloable_kworker, adxl->work);
        virtual_dev_ioctl(adxl->i2c_bus, I2C_IOCTL_TRANSFER_TX, adxl->i2c_xfer->transfer_args);
        self->fun->trigger_event(self, ADXL_INIT_START, NULL);
        break;
    }
    default: break;
    }
}

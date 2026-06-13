/**
 * CAN 中断模式
 */
#include "can.h"
#include "../hal/hal_can.h"

/* ── 中断处理 ── */
static bool can_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    const can_config_t *conf = dev->info->conf;
    if (conf->id >= CAN_MAX) return true;

    xCAN_TypeDef *can = CANx[conf->id];
    uint32_t msr = hal_can_get_msr(can);
    uint32_t ier = hal_can_get_ier(can);

    if ((ier & xCAN_IER_TMEIE) && (msr & xCAN_MSR_TXM))
        dev->fun->trigger_event(dev, CAN_TX_DONE, dev->arg);

    if ((ier & xCAN_IER_FMPIE0) && (hal_can_get_rf0r(can) & 0x03))
        dev->fun->trigger_event(dev, CAN_RX_DONE, dev->arg);

    if ((ier & xCAN_IER_FMPIE1) && (hal_can_get_rf1r(can) & 0x03))
        dev->fun->trigger_event(dev, CAN_RX_DONE, dev->arg);

    if ((ier & xCAN_IER_ERRIE) && (msr & (1 << 2)))
        dev->fun->trigger_event(dev, CAN_XFER_ERROR, dev->arg);

    return true;
}

/* ── dev_init ── */
void can_it_dev_init(Device *self) {
    const can_config_t *conf = self->info->conf;
    if (!conf->it_enable) return;

    uint32_t ier = 0;
    xCAN_TypeDef *can = CANx[conf->id];

    if (conf->it_enable & CAN_IT_TME) {
        ier |= xCAN_IER_TMEIE;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
            CAN1_TX_IRQ + conf->id * 4);
        self->irq_conf->handler = can_irq_handler;
        self->irq_conf->arg = self;
        self->fun->config_irq(self, self->irq_conf);
    }
    if (conf->it_enable & CAN_IT_FMP0) {
        ier |= xCAN_IER_FMPIE0;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
            CAN1_RX0_IRQ + conf->id * 4);
        self->irq_conf->handler = can_irq_handler;
        self->irq_conf->arg = self;
        self->fun->config_irq(self, self->irq_conf);
    }
    if (conf->it_enable & CAN_IT_FMP1) {
        ier |= xCAN_IER_FMPIE1;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
            CAN1_RX1_IRQ + conf->id * 4);
        self->irq_conf->handler = can_irq_handler;
        self->irq_conf->arg = self;
        self->fun->config_irq(self, self->irq_conf);
    }
    if (conf->it_enable & CAN_IT_ERR) {
        ier |= xCAN_IER_ERRIE;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list,
            CAN1_SCE_IRQ + conf->id * 4);
        self->irq_conf->handler = can_irq_handler;
        self->irq_conf->arg = self;
        self->fun->config_irq(self, self->irq_conf);
    }
    hal_can_set_ier(can, ier);
}

/* ── dev_write: 发送 CAN 消息 ── */
void can_it_write(Device *self, const void *buf, size_t count) {
    const can_config_t *conf = self->info->conf;
    if (count < sizeof(can_msg_t) || !buf) return;
    const can_msg_t *msg = (const can_msg_t *)buf;
    xCAN_TypeDef *can = CANx[conf->id];

    int mbox = hal_can_find_free_mailbox(can);
    if (mbox < 0) return;

    hal_can_write_mailbox(can, mbox, msg);
    hal_can_request_send(can, mbox);
    self->fun->trigger_event(self, CAN_TX_START, self->arg);
}

/* ── dev_read: 接收 CAN 消息 ── */
size_t can_it_read(Device *self, void *buf, size_t count) {
    const can_config_t *conf = self->info->conf;
    if (count < sizeof(can_msg_t) || !buf) return 0;

    xCAN_TypeDef *can = CANx[conf->id];
    if (!hal_can_rx_fifo_has_msg(can, CAN_FIFO0)) return 0;

    hal_can_read_mailbox(can, CAN_FIFO0, (can_msg_t *)buf);
    hal_can_release_fifo(can, CAN_FIFO0);
    self->fun->trigger_event(self, CAN_RX_START, self->arg);
    return sizeof(can_msg_t);
}

/* ── dev_ioctl ── */
void can_it_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

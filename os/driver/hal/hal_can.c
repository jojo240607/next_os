//
// Created by zhiwei.gong on 2026/5/14.
//

#include "hal_can.h"
#include <string.h>

xCAN_TypeDef* const CANx[CAN_MAX] = {
        (xCAN_TypeDef*)xCAN1_BASE,
        (xCAN_TypeDef*)xCAN2_BASE
};

/* ════ 初始化模式 ════ */
void hal_can_enter_init_mode(xCAN_TypeDef *can) {
    can->MCR |= xCAN_MCR_INRQ;
    while (!(can->MSR & xCAN_MSR_INAK));
}

void hal_can_exit_init_mode(xCAN_TypeDef *can) {
    can->MCR &= ~xCAN_MCR_INRQ;
    while (can->MSR & xCAN_MSR_INAK);
}

/* ════ MCR ════ */
void hal_can_config_mcr(xCAN_TypeDef *can, bool abom, bool awum, bool nart) {
    uint32_t mcr = can->MCR;
    if (abom) mcr |= xCAN_MCR_ABOM;  else mcr &= ~xCAN_MCR_ABOM;
    if (awum) mcr |= xCAN_MCR_AWUM;  else mcr &= ~xCAN_MCR_AWUM;
    if (nart) mcr |= xCAN_MCR_NART;  else mcr &= ~xCAN_MCR_NART;
    can->MCR = mcr;
}

/* ════ BTR ════ */
void hal_can_config_btr(xCAN_TypeDef *can, can_mode_t mode,
                        uint8_t sjw, uint8_t bs1, uint8_t bs2, uint32_t prescaler) {
    uint32_t btr = 0;
    btr |= ((sjw - 1) & 0x03) << 24;
    btr |= ((bs2 - 1) & 0x07) << 20;
    btr |= ((bs1 - 1) & 0x0F) << 16;
    btr |= ((prescaler - 1) & 0x3FF) << 0;
    if (mode == CAN_MODE_LOOPBACK)        btr |= (1 << 30);
    else if (mode == CAN_MODE_SILENT)     btr |= (1 << 31);
    else if (mode == CAN_MODE_SILENT_LOOP) btr |= (1 << 30) | (1 << 31);
    can->BTR = btr;
}

void hal_can_set_btr(xCAN_TypeDef *can, uint32_t btr) { can->BTR = btr; }

/* ════ 过滤器 ════ */
void hal_can_filter_init_enter(xCAN_TypeDef *can) { can->FMR |= xCAN_FMR_FINIT; }
void hal_can_filter_init_exit(xCAN_TypeDef *can)  { can->FMR &= ~xCAN_FMR_FINIT; }

void hal_can_config_filter_bank(xCAN_TypeDef *can, const uint8_t bank,
                                can_filter_mode_t mode, can_filter_scale_t scale,
                                can_fifo_t fifo, bool active,
                                uint32_t id_high, uint32_t id_low) {
    if (bank >= 28) return;
    if (mode == CAN_FILTER_LIST_MODE) can->FM1R |= (1UL << bank);
    else                              can->FM1R &= ~(1UL << bank);
    if (scale == CAN_FILTER_32BIT)    can->FS1R |= (1UL << bank);
    else                              can->FS1R &= ~(1UL << bank);
    if (fifo == CAN_FIFO1)            can->FFA1R |= (1UL << bank);
    else                              can->FFA1R &= ~(1UL << bank);
    can->FILTER[bank][0] = id_high;
    can->FILTER[bank][1] = id_low;
    if (active) can->FA1R |= (1UL << bank);
    else        can->FA1R &= ~(1UL << bank);
}

/* ════ 发送 ════ */
int hal_can_find_free_mailbox(xCAN_TypeDef *can) {
    uint32_t tsr = can->TSR;
    if (!(tsr & (1 << 26))) return 0;
    if (!(tsr & (1 << 27))) return 1;
    if (!(tsr & (1 << 28))) return 2;
    return -1;
}

void hal_can_write_mailbox(xCAN_TypeDef *can, int mbox, const can_msg_t *msg) {
    uint32_t rir = 0;
    if (msg->id_type == CAN_ID_EXTENDED) rir = (msg->id & 0x1FFFFFFF) << 3;
    else                                  rir = (msg->id & 0x7FF) << 21;
    if (msg->frame_type == CAN_FRAME_REMOTE) rir |= (1 << 1);
    if (msg->id_type == CAN_ID_EXTENDED)     rir |= (1 << 2);

    uint32_t rdlr = (uint32_t)msg->data[0] << 0 | (uint32_t)msg->data[1] << 8
                  | (uint32_t)msg->data[2] << 16 | (uint32_t)msg->data[3] << 24;
    uint32_t rdhr = (uint32_t)msg->data[4] << 0 | (uint32_t)msg->data[5] << 8
                  | (uint32_t)msg->data[6] << 16 | (uint32_t)msg->data[7] << 24;

    can->TX[mbox][0] = rir;
    can->TX[mbox][1] = (msg->dlc & 0x0F);
    can->TX[mbox][2] = rdlr;
    can->TX[mbox][3] = rdhr;
}

void hal_can_request_send(xCAN_TypeDef *can, int mbox) {
    can->TX[mbox][0] |= (1 << 0);   // TXRQ
}

/* ════ 接收 ════ */
bool hal_can_rx_fifo_has_msg(xCAN_TypeDef *can, can_fifo_t fifo) {
    volatile uint32_t *fifo_reg = (fifo == CAN_FIFO0) ? &can->RF0R : &can->RF1R;
    return (*fifo_reg & 0x03) != 0;
}

void hal_can_release_fifo(xCAN_TypeDef *can, can_fifo_t fifo) {
    volatile uint32_t *fifo_reg = (fifo == CAN_FIFO0) ? &can->RF0R : &can->RF1R;
    *fifo_reg |= (1 << 5);   // RFOM
}

void hal_can_read_mailbox(xCAN_TypeDef *can, can_fifo_t fifo, can_msg_t *msg) {
    uint8_t idx = (fifo == CAN_FIFO0) ? 0 : 1;
    uint32_t rir  = can->RX[idx][0];
    uint32_t rdtr = can->RX[idx][1];
    uint32_t rdlr = can->RX[idx][2];
    uint32_t rdhr = can->RX[idx][3];

    if (rir & (1 << 2)) {
        msg->id_type = CAN_ID_EXTENDED;
        msg->id = (rir >> 3) & 0x1FFFFFFF;
    } else {
        msg->id_type = CAN_ID_STANDARD;
        msg->id = (rir >> 21) & 0x7FF;
    }
    msg->frame_type = (rir & (1 << 1)) ? CAN_FRAME_REMOTE : CAN_FRAME_DATA;
    msg->dlc = rdtr & 0x0F;
    if (msg->dlc > 8) msg->dlc = 8;

    msg->data[0] = (rdlr >> 0) & 0xFF;  msg->data[1] = (rdlr >> 8) & 0xFF;
    msg->data[2] = (rdlr >> 16) & 0xFF; msg->data[3] = (rdlr >> 24) & 0xFF;
    msg->data[4] = (rdhr >> 0) & 0xFF;  msg->data[5] = (rdhr >> 8) & 0xFF;
    msg->data[6] = (rdhr >> 16) & 0xFF; msg->data[7] = (rdhr >> 24) & 0xFF;
}

/* ════ 状态 ════ */
bool hal_can_is_tx_mailbox_free(xCAN_TypeDef *can) {
    uint32_t tsr = can->TSR;
    return (!(tsr & (1 << 26))) || (!(tsr & (1 << 27))) || (!(tsr & (1 << 28)));
}

uint8_t hal_can_get_rx_fifo_count(xCAN_TypeDef *can, can_fifo_t fifo) {
    volatile uint32_t *fifo_reg = (fifo == CAN_FIFO0) ? &can->RF0R : &can->RF1R;
    return (*fifo_reg) & 0x03;
}

/* ════ 休眠/唤醒 ════ */
void hal_can_sleep(xCAN_TypeDef *can) {
    can->MCR |= xCAN_MCR_SLEEP;
    while (!(can->MSR & xCAN_MSR_SLAK));
}

void hal_can_wakeup(xCAN_TypeDef *can) {
    can->MCR &= ~xCAN_MCR_SLEEP;
    while (can->MSR & xCAN_MSR_SLAK);
}

/* ════ 中断 ════ */
void hal_can_set_ier(xCAN_TypeDef *can, uint32_t ier) { can->IER = ier; }
uint32_t hal_can_get_msr(xCAN_TypeDef *can) { return can->MSR; }
uint32_t hal_can_get_ier(xCAN_TypeDef *can) { return can->IER; }
uint32_t hal_can_get_rf0r(xCAN_TypeDef *can) { return can->RF0R; }
uint32_t hal_can_get_rf1r(xCAN_TypeDef *can) { return can->RF1R; }

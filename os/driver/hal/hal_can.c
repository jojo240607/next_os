//
// Created by zhiwei.gong on 2026/5/14.
//

#include "hal_can.h"

xCAN_TypeDef* const CANx[CAN_MAX] = {
        (xCAN_TypeDef*)xCAN1_BASE,
        (xCAN_TypeDef*)xCAN2_BASE
};

/* ---------- 辅助：从 FIFO 邮箱读取消息 ---------- */
void read_mailbox(xCAN_TypeDef *can, can_fifo_t fifo, can_msg_t *msg)
{
    uint8_t idx = (fifo == CAN_FIFO0) ? 0 : 1;
    uint32_t rir  = can->RX[idx][0];
    uint32_t rdtr = can->RX[idx][1];
    uint32_t rdlr = can->RX[idx][2];
    uint32_t rdhr = can->RX[idx][3];

    if (rir & (1 << 2)) {  // 扩展 ID
        msg->id_type = CAN_ID_EXTENDED;
        msg->id = (rir >> 3) & 0x1FFFFFFF;
    } else {
        msg->id_type = CAN_ID_STANDARD;
        msg->id = (rir >> 21) & 0x7FF;
    }
    msg->frame_type = (rir & (1 << 1)) ? CAN_FRAME_REMOTE : CAN_FRAME_DATA;
    msg->dlc = rdtr & 0x0F;
    if (msg->dlc > 8) msg->dlc = 8;

    msg->data[0] = (rdlr >> 0) & 0xFF;
    msg->data[1] = (rdlr >> 8) & 0xFF;
    msg->data[2] = (rdlr >> 16) & 0xFF;
    msg->data[3] = (rdlr >> 24) & 0xFF;
    msg->data[4] = (rdhr >> 0) & 0xFF;
    msg->data[5] = (rdhr >> 8) & 0xFF;
    msg->data[6] = (rdhr >> 16) & 0xFF;
    msg->data[7] = (rdhr >> 24) & 0xFF;
}


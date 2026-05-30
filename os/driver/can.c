#include "can.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "common/rcc.h"
#include "../log/log.h"

dev_init_override(can_dev_init_impl);

dev_ioctl_override(can_dev_ioctl_impl);

// 析构函数声明
static void can_destroy(Can* self);
static bool can_irq_handler_impl(nvic_irq_t *irq_conf);
// TODO: 初始化数据成员
static const CanFun can_fun = {
    .destroy = can_destroy,
};
// 构造函数实现
Can* can_create(const device_info_t *info) {
    Can* obj = (Can*)os_malloc(sizeof(Can));
    if (obj) {
        memset(obj, 0, sizeof(Can));
        can_init(obj, info);
    }
    return obj;
}

void can_init(Can* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(can_fun);
    // TODO: 初始化派生类特有成员
	def_dev_ioctl(self) = can_dev_ioctl_impl;
	def_dev_init(self) = can_dev_init_impl;
}

void can_deinit(Can* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void can_destroy(Can* self) {
    if (self != NULL) {
        can_deinit(self);
        os_free(self);
    }
}

// dev_ioctl method
dev_ioctl_override(can_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Can *can = (Can *)self;

    //params , int cmd, void *arg

}


static bool can_irq_handler_impl(nvic_irq_t *irq_conf) {
    Can *can = GET_CAN(irq_conf->arg);
    const can_config_t *conf = GET_DEVICE(can)->info->conf;
    if (conf->id >= CAN_MAX) {
        return true;
    }
    xCAN_TypeDef *can_ctrl = CANx[conf->id];
    uint32_t msr = can_ctrl->MSR;
    uint32_t ier = can_ctrl->IER;

    // 发送邮箱空
    if ((ier & xCAN_IER_TMEIE) && (msr & xCAN_MSR_TXM)) {

    }
    // FIFO0 消息挂起
    if ((ier & xCAN_IER_FMPIE0) && (can_ctrl->RF0R & 0x03)) {

    }
    // FIFO1 消息挂起
    if ((ier & xCAN_IER_FMPIE1) && (can_ctrl->RF1R & 0x03)) {

    }
    // 错误中断
    if ((ier & xCAN_IER_ERRIE) && (msr & (1 << 2))) {   // ERRI

    }
}

/* ===================================================================
   发送消息
   =================================================================== */
int can_send(can_id_t id, const can_msg_t *msg)
{
    if (id >= CAN_MAX || !msg) {
        return -1;
    }
    xCAN_TypeDef *can = CANx[id];

    // 寻找空闲邮箱
    uint32_t tsr = can->TSR;
    int mbox = -1;
    if (!(tsr & (1 << 26))) mbox = 0;       // TME0
    else if (!(tsr & (1 << 27))) mbox = 1;  // TME1
    else if (!(tsr & (1 << 28))) mbox = 2;  // TME2
    if (mbox < 0) {
        return -1;
    }

    // 构建 RIR
    uint32_t rir = 0;
    if (msg->id_type == CAN_ID_EXTENDED) {
        rir = (msg->id & 0x1FFFFFFF) << 3;
    } else {
        rir = (msg->id & 0x7FF) << 21;
    }
    if (msg->frame_type == CAN_FRAME_REMOTE) {
        rir |= (1 << 1);   // RTR
    }
    if (msg->id_type == CAN_ID_EXTENDED) {
        rir |= (1 << 2);   // IDE
    }

    // 构建 RDTR
    uint32_t rdtr = (msg->dlc & 0x0F);

    // 构建数据字
    uint32_t rdlr = 0, rdhr = 0;
    rdlr |= (uint32_t)msg->data[0] << 0;
    rdlr |= (uint32_t)msg->data[1] << 8;
    rdlr |= (uint32_t)msg->data[2] << 16;
    rdlr |= (uint32_t)msg->data[3] << 24;
    rdhr |= (uint32_t)msg->data[4] << 0;
    rdhr |= (uint32_t)msg->data[5] << 8;
    rdhr |= (uint32_t)msg->data[6] << 16;
    rdhr |= (uint32_t)msg->data[7] << 24;

    // 填入邮箱
    can->TX[mbox][0] = rir;
    can->TX[mbox][1] = rdtr;
    can->TX[mbox][2] = rdlr;
    can->TX[mbox][3] = rdhr;

    // 请求发送
    can->TX[mbox][0] |= (1 << 0);   // TXRQ

    return 0;
}

/* ===================================================================
   接收消息
   =================================================================== */
int can_receive(can_id_t id, can_fifo_t fifo, can_msg_t *msg)
{
    if (id >= CAN_MAX || !msg) {
        return -1;
    }
    xCAN_TypeDef *can = CANx[id];

    volatile uint32_t *fifo_reg = (fifo == CAN_FIFO0) ? &can->RF0R : &can->RF1R;
    if ((*fifo_reg & 0x03) == 0) {
        return -1;   // 没有消息
    }

    read_mailbox(can, fifo, msg);

    // 释放邮箱 (RFOM)
    *fifo_reg |= (1 << 5);

    return 0;
}

/* ===================================================================
   状态查询
   =================================================================== */
bool can_is_tx_mailbox_free(can_id_t id)
{
    if (id >= CAN_MAX) {
        return false;
    }
    uint32_t tsr = CANx[id]->TSR;
    return (!(tsr & (1 << 26))) || (!(tsr & (1 << 27))) || (!(tsr & (1 << 28)));
}

uint8_t can_get_rx_fifo_count(can_id_t id, can_fifo_t fifo)
{
    if (id >= CAN_MAX) {
        return 0;
    }
    volatile uint32_t *fifo_reg = (fifo == CAN_FIFO0) ? &CANx[id]->RF0R : &CANx[id]->RF1R;
    return (*fifo_reg) & 0x03;
}

/* ===================================================================
   休眠/唤醒
   =================================================================== */
int can_sleep(can_id_t id)
{
    if (id >= CAN_MAX) {
        return -1;
    }
    CANx[id]->MCR |= xCAN_MCR_SLEEP;
    while (!(CANx[id]->MSR & xCAN_MSR_SLAK));
    return 0;
}

int can_wakeup(can_id_t id)
{
    if (id >= CAN_MAX) {
        return -1;
    }
    CANx[id]->MCR &= ~xCAN_MCR_SLEEP;
    while (CANx[id]->MSR & xCAN_MSR_SLAK);
    return 0;
}


// dev_init method
dev_init_override(can_dev_init_impl) {
    // TODO: add dev_init method
    Can *can = (Can *)self;
    const can_config_t *conf = self->info->conf;
    //params 
    if (!conf || conf->id >= CAN_MAX) {
        return;
    }

    xCAN_TypeDef *can_ctrl = CANx[conf->id];

    // 1. 使能时钟
    if (conf->id == CAN_1)
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_CAN1EN);   // APB1 位25 CAN1
    else
        rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_CAN2EN);   // APB1 位26 CAN2

    // 2. 配置引脚 (AF9)
    pin_config_t pins[2] = {
            {.mode = PIN_MODE_AF,
                    .otype = PIN_OTYPE_PP,
                    .ospeed = PIN_OSPEED_HIGH,
                    .pupd = PIN_PUPD_NONE,
                    .af = conf->pins.can_tx },
            {.mode = PIN_MODE_AF,
                    .otype = PIN_OTYPE_PP,
                    .ospeed = PIN_OSPEED_HIGH,
                    .pupd = PIN_PUPD_NONE,
                    .af =conf->pins.can_rx }
    };
    if (pinmux_request_group(pins, 2) != PINMUX_SUCCESS) {
        LOG_DEBUG("can", "init pinmux error!");
        return;
    }

    // 3. 进入初始化模式
    can_ctrl->MCR |= xCAN_MCR_INRQ;
    while (!(can_ctrl->MSR & xCAN_MSR_INAK));

    // 4. 配置主控制寄存器
    uint32_t mcr = can_ctrl->MCR;
    if (conf->auto_bus_off) {
        mcr |= xCAN_MCR_ABOM;
    } else {
        mcr &= ~xCAN_MCR_ABOM;
    }
    if (conf->auto_wakeup) {
        mcr |= xCAN_MCR_AWUM;
    } else {
        mcr &= ~xCAN_MCR_AWUM;
    }
    if (conf->no_auto_retrans) {
        mcr |= xCAN_MCR_NART;
    } else {
        mcr &= ~xCAN_MCR_NART;
    }
    can_ctrl->MCR = mcr;

    // 5. 配置位时序 (以 pclk1 为基准)
    uint32_t btr = 0;
    btr |= ((conf->sjw - 1) & 0x03) << 24;      // SJW
    btr |= ((conf->bs2 - 1) & 0x07) << 20;      // TS2
    btr |= ((conf->bs1 - 1) & 0x0F) << 16;      // TS1
    btr |= ((conf->prescaler - 1) & 0x3FF) << 0; // BRP
    // 模式
    if (conf->mode == CAN_MODE_LOOPBACK) {
        btr |= (1 << 30);   // LBKM
    } else if (conf->mode == CAN_MODE_SILENT) {
        btr |= (1 << 31);   // SILM
    } else if (conf->mode == CAN_MODE_SILENT_LOOP) {
        btr |= (1 << 30) | (1 << 31);
    }
    can_ctrl->BTR = btr;

    // 6. 配置过滤器
    if (conf->num_filters > 0 && *conf->filters) {
        can_ctrl->FMR |= xCAN_FMR_FINIT;          // 进入过滤器初始化模式
        for (int i = 0; i < conf->num_filters; i++) {
            const can_filter_config_t *f = conf->filters[i];
            uint8_t bank = f->bank;
            if (bank >= 28) {
                continue;
            }

            // 模式 (屏蔽/列表)
            if (f->mode == CAN_FILTER_LIST_MODE) {
                can_ctrl->FM1R |= (1UL << bank);
            } else {
                can_ctrl->FM1R &= ~(1UL << bank);
            }
            // 位宽
            if (f->scale == CAN_FILTER_32BIT) {
                can_ctrl->FS1R |= (1UL << bank);
            } else {
                can_ctrl->FS1R &= ~(1UL << bank);
            }
            // FIFO 分配
            if (f->fifo == CAN_FIFO1) {
                can_ctrl->FFA1R |= (1UL << bank);
            } else {
                can_ctrl->FFA1R &= ~(1UL << bank);
            }
            // 写入过滤值
            can_ctrl->FILTER[bank][0] = f->id_high;
            can_ctrl->FILTER[bank][1] = f->id_low;

            // 激活
            if (f->active) {
                can_ctrl->FA1R |= (1UL << bank);
            } else {
                can_ctrl->FA1R &= ~(1UL << bank);
            }
        }
        can_ctrl->FMR &= ~xCAN_FMR_FINIT;         // 退出过滤器初始化模式
    }

    // 7. 配置中断
    if (conf->it_enable) {
        self->irq_conf->handler = can_irq_handler_impl;
        //can_callbacks[conf->id] = conf->callback;
        uint32_t ier = 0;
        if (conf->it_enable & CAN_IT_TME)  {
            ier |= xCAN_IER_TMEIE;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, CAN1_TX_IRQ + conf->id * 4);
            self->fun->config_irq(self, self->irq_conf);
        }
        if (conf->it_enable & CAN_IT_FMP0) {
            ier |= xCAN_IER_FMPIE0;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, CAN1_RX0_IRQ + conf->id * 4);
            self->fun->config_irq(self, self->irq_conf);
        }
        if (conf->it_enable & CAN_IT_FMP1) {
            ier |= xCAN_IER_FMPIE1;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, CAN1_RX1_IRQ + conf->id * 4);
            self->fun->config_irq(self, self->irq_conf);
        }
        if (conf->it_enable & CAN_IT_ERR)  {
            ier |= xCAN_IER_ERRIE;
            self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, CAN1_SCE_IRQ + conf->id * 4);
            self->fun->config_irq(self, self->irq_conf);
        }
        can_ctrl->IER = ier;
    }

    // 8. 退出初始化模式
    can_ctrl->MCR &= ~xCAN_MCR_INRQ;
    while (can_ctrl->MSR & xCAN_MSR_INAK);

    //can_inited[conf->id] = true;
}


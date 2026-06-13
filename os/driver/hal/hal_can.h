#ifndef STM32F4DISCOVERY_HAL_CAN_H
#define STM32F4DISCOVERY_HAL_CAN_H
#include "stdint.h"
#include <stdbool.h>

/* CAN 实例 */
typedef enum : uint8_t {
    CAN_1 = 0,
    CAN_2 = 1,
    CAN_MAX
} can_id_t;

/* 工作模式 */
typedef enum : uint8_t {
    CAN_MODE_NORMAL       = 0,
    CAN_MODE_LOOPBACK     = 1,
    CAN_MODE_SILENT       = 2,
    CAN_MODE_SILENT_LOOP  = 3
} can_mode_t;

/* 帧 ID 类型 */
typedef enum : uint8_t {
    CAN_ID_STANDARD = 0,
    CAN_ID_EXTENDED = 1
} can_id_type_t;

/* 帧类型 (数据 / 远程) */
typedef enum : uint8_t {
    CAN_FRAME_DATA   = 0,
    CAN_FRAME_REMOTE = 1
} can_frame_type_t;

/* 过滤器模式 */
typedef enum : uint8_t {
    CAN_FILTER_MASK_MODE = 0,
    CAN_FILTER_LIST_MODE = 1
} can_filter_mode_t;

/* 过滤器位宽 */
typedef enum : uint8_t {
    CAN_FILTER_16BIT = 0,
    CAN_FILTER_32BIT = 1
} can_filter_scale_t;

/* FIFO 选择 */
typedef enum : uint8_t {
    CAN_FIFO0 = 0,
    CAN_FIFO1 = 1
} can_fifo_t;

/* 中断使能选项 */
typedef enum : uint8_t {
    CAN_IT_TME     = (1 << 0),
    CAN_IT_FMP0    = (1 << 1),
    CAN_IT_FMP1    = (1 << 2),
    CAN_IT_ERR     = (1 << 3),
} can_it_t;

/* 中断事件 */
typedef enum : uint8_t {
    CAN_EVT_TX_MAILBOX = 0,
    CAN_EVT_RX_FIFO0   = 1,
    CAN_EVT_RX_FIFO1   = 2,
    CAN_EVT_ERROR      = 3
} can_event_t;

/* CAN 消息帧 */
typedef struct {
    uint32_t         id;
    can_id_type_t    id_type;
    can_frame_type_t frame_type;
    uint8_t          dlc;
    uint8_t          data[8];
} can_msg_t;

/* ---------- CAN 寄存器 ---------- */
#define xCAN1_BASE  0x40006400UL
#define xCAN2_BASE  0x40006800UL

typedef struct {
    volatile uint32_t MCR;
    volatile uint32_t MSR;
    volatile uint32_t TSR;
    volatile uint32_t RF0R;
    volatile uint32_t RF1R;
    volatile uint32_t IER;
    volatile uint32_t ESR;
    volatile uint32_t BTR;
    uint32_t reserved0[88];
    volatile uint32_t TX[3][4];
    volatile uint32_t RX[2][4];
    uint32_t reserved1[12];
    volatile uint32_t FMR;
    volatile uint32_t FM1R;
    uint32_t reserved2;
    volatile uint32_t FS1R;
    uint32_t reserved3;
    volatile uint32_t FFA1R;
    uint32_t reserved4;
    volatile uint32_t FA1R;
    uint32_t reserved5[8];
    volatile uint32_t FILTER[28][2];
} xCAN_TypeDef;

/* ---------- 寄存器位 ---------- */
#define xCAN_MCR_INRQ    (1 << 0)
#define xCAN_MCR_SLEEP   (1 << 1)
#define xCAN_MCR_TXFP    (1 << 2)
#define xCAN_MCR_RFLM    (1 << 3)
#define xCAN_MCR_NART    (1 << 4)
#define xCAN_MCR_AWUM    (1 << 5)
#define xCAN_MCR_ABOM    (1 << 6)
#define xCAN_MCR_DBF     (1 << 16)

#define xCAN_MSR_INAK    (1 << 0)
#define xCAN_MSR_SLAK    (1 << 1)
#define xCAN_MSR_ERRI    (1 << 2)
#define xCAN_MSR_WKUI    (1 << 3)
#define xCAN_MSR_TXM     (1 << 8)
#define xCAN_MSR_RXM     (1 << 9)

#define xCAN_IER_TMEIE   (1 << 0)
#define xCAN_IER_FMPIE0  (1 << 1)
#define xCAN_IER_FMPIE1  (1 << 2)
#define xCAN_IER_ERRIE   (1 << 5)

#define xCAN_FMR_FINIT   (1 << 0)

extern xCAN_TypeDef* const CANx[CAN_MAX];

/* ── 初始化模式 ── */
void hal_can_enter_init_mode(xCAN_TypeDef *can);
void hal_can_exit_init_mode(xCAN_TypeDef *can);

/* ── 主控制寄存器 ── */
void hal_can_config_mcr(xCAN_TypeDef *can, bool abom, bool awum, bool nart);

/* ── 位时序 ── */
void hal_can_config_btr(xCAN_TypeDef *can, can_mode_t mode,
                        uint8_t sjw, uint8_t bs1, uint8_t bs2, uint32_t prescaler);
void hal_can_set_btr(xCAN_TypeDef *can, uint32_t btr);

/* ── 过滤器 ── */
void hal_can_filter_init_enter(xCAN_TypeDef *can);
void hal_can_filter_init_exit(xCAN_TypeDef *can);
void hal_can_config_filter_bank(xCAN_TypeDef *can, const uint8_t bank,
                                can_filter_mode_t mode, can_filter_scale_t scale,
                                can_fifo_t fifo, bool active,
                                uint32_t id_high, uint32_t id_low);

/* ── 发送 ── */
int  hal_can_find_free_mailbox(xCAN_TypeDef *can);
void hal_can_write_mailbox(xCAN_TypeDef *can, int mbox, const can_msg_t *msg);
void hal_can_request_send(xCAN_TypeDef *can, int mbox);

/* ── 接收 ── */
bool hal_can_rx_fifo_has_msg(xCAN_TypeDef *can, can_fifo_t fifo);
void hal_can_release_fifo(xCAN_TypeDef *can, can_fifo_t fifo);
void hal_can_read_mailbox(xCAN_TypeDef *can, can_fifo_t fifo, can_msg_t *msg);

/* ── 状态查询 ── */
bool    hal_can_is_tx_mailbox_free(xCAN_TypeDef *can);
uint8_t hal_can_get_rx_fifo_count(xCAN_TypeDef *can, can_fifo_t fifo);

/* ── 休眠/唤醒 ── */
void hal_can_sleep(xCAN_TypeDef *can);
void hal_can_wakeup(xCAN_TypeDef *can);

/* ── 中断控制/状态 ── */
void     hal_can_set_ier(xCAN_TypeDef *can, uint32_t ier);
uint32_t hal_can_get_msr(xCAN_TypeDef *can);
uint32_t hal_can_get_ier(xCAN_TypeDef *can);
uint32_t hal_can_get_rf0r(xCAN_TypeDef *can);
uint32_t hal_can_get_rf1r(xCAN_TypeDef *can);

#endif //STM32F4DISCOVERY_HAL_CAN_H

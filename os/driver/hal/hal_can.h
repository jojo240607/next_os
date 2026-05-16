//
// Created by zhiwei.gong on 2026/5/14.
//

#ifndef STM32F4DISCOVERY_HAL_CAN_H
#define STM32F4DISCOVERY_HAL_CAN_H
#include "stdint.h"

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
    CAN_FILTER_MASK_MODE = 0,   // 屏蔽位模式
    CAN_FILTER_LIST_MODE = 1    // 标识符列表模式
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
    CAN_IT_TME     = (1 << 0),   // 发送邮箱空中断
    CAN_IT_FMP0    = (1 << 1),   // FIFO0 消息挂起中断
    CAN_IT_FMP1    = (1 << 2),   // FIFO1 消息挂起中断
    CAN_IT_ERR     = (1 << 3),   // 错误和状态变化中断
} can_it_t;


/* 中断事件 */
typedef enum : uint8_t {
    CAN_EVT_TX_MAILBOX = 0,      // 发送邮箱空
    CAN_EVT_RX_FIFO0   = 1,      // FIFO0 收到新消息
    CAN_EVT_RX_FIFO1   = 2,      // FIFO1 收到新消息
    CAN_EVT_ERROR      = 3       // 总线错误或状态变化
} can_event_t;

/* CAN 消息帧 */
typedef struct {
    uint32_t         id;         // 标准 (11位) 或扩展 (29位) ID
    can_id_type_t    id_type;
    can_frame_type_t frame_type;
    uint8_t          dlc;        // 数据长度 (0..8)
    uint8_t          data[8];
} can_msg_t;

/* ---------- CAN 寄存器基址 ---------- */
#define xCAN1_BASE  0x40006400UL
#define xCAN2_BASE  0x40006800UL
/* ---------- CAN 寄存器结构 ---------- */
typedef struct {
    volatile uint32_t MCR;          // 主控制寄存器
    volatile uint32_t MSR;          // 主状态寄存器
    volatile uint32_t TSR;          // 发送状态寄存器
    volatile uint32_t RF0R;         // 接收 FIFO0
    volatile uint32_t RF1R;         // 接收 FIFO1
    volatile uint32_t IER;          // 中断使能寄存器
    volatile uint32_t ESR;          // 错误状态寄存器
    volatile uint32_t BTR;          // 位时序寄存器
    uint32_t reserved0[88];
    volatile uint32_t TX[3][4];     // 发送邮箱 (3 个邮箱，每个 4 字)
    volatile uint32_t RX[2][4];     // 接收 FIFO (2 个，每个 4 字)
    uint32_t reserved1[12];
    volatile uint32_t FMR;          // 过滤器主寄存器
    volatile uint32_t FM1R;         // 过滤器模式寄存器
    uint32_t reserved2;
    volatile uint32_t FS1R;         // 过滤器位宽寄存器
    uint32_t reserved3;
    volatile uint32_t FFA1R;        // 过滤器 FIFO 分配寄存器
    uint32_t reserved4;
    volatile uint32_t FA1R;         // 过滤器激活寄存器
    uint32_t reserved5[8];
    volatile uint32_t FILTER[28][2]; // 28 组过滤器，每组 2 字
} xCAN_TypeDef;



/* ---------- 控制寄存器位 ---------- */
#define xCAN_MCR_INRQ    (1 << 0)     // 初始化请求
#define xCAN_MCR_SLEEP   (1 << 1)     // 睡眠请求
#define xCAN_MCR_TXFP    (1 << 2)     // 发送优先级由标识符决定
#define xCAN_MCR_RFLM    (1 << 3)     // 接收 FIFO 锁定模式
#define xCAN_MCR_NART    (1 << 4)     // 禁止自动重传
#define xCAN_MCR_AWUM    (1 << 5)     // 自动唤醒
#define xCAN_MCR_ABOM    (1 << 6)     // 自动离线恢复
#define xCAN_MCR_DBF     (1 << 16)    // 调试冻结

/* ---------- 状态寄存器位 ---------- */
#define xCAN_MSR_INAK    (1 << 0)     // 初始化确认
#define xCAN_MSR_SLAK    (1 << 1)     // 睡眠确认
#define xCAN_MSR_ERRI    (1 << 2)     // 错误状态
#define xCAN_MSR_WKUI    (1 << 3)     // 唤醒中断
#define xCAN_MSR_TXM     (1 << 8)     // 发送邮箱空标志
#define xCAN_MSR_RXM     (1 << 9)     // 接收 FIFO 非空标志

/* ---------- 中断使能寄存器位 ---------- */
#define xCAN_IER_TMEIE   (1 << 0)     // 发送邮箱空中断使能
#define xCAN_IER_FMPIE0  (1 << 1)     // FIFO0 消息挂起中断使能
#define xCAN_IER_FMPIE1  (1 << 2)     // FIFO1 消息挂起中断使能
#define xCAN_IER_ERRIE   (1 << 5)     // 错误中断使能

/* ---------- 过滤器主寄存器位 ---------- */
#define xCAN_FMR_FINIT   (1 << 0)     // 初始化模式

extern xCAN_TypeDef* const CANx[CAN_MAX];

void read_mailbox(xCAN_TypeDef *can, can_fifo_t fifo, can_msg_t *msg);

#endif //STM32F4DISCOVERY_HAL_CAN_H

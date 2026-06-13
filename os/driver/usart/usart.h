#ifndef USART_H
#define USART_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../common/dma.h"
#include "../hal/hal_usart.h"
#include "../../common/ringbuf.h"
#include "../../scheduler/semaphore.h"

#define GET_USART(obj) ((Usart *)obj)
#define DEFAULT_RX_BUFFER (256)

/* UART 事件 (用于 listener 模式) */
typedef enum : uint8_t {
    UART_TX_START = 1,
    UART_TX_DONE,
    UART_RX_START,
    UART_RX_DONE,
    UART_XFER_ERROR,
} uart_dev_event_t;

// 派生类声明
typedef struct _Usart Usart;
typedef struct _UsartFun UsartFun;
// 类成员函数结构
struct _UsartFun {
    void (*destroy)(Usart* self);
};



/* UART DMA 配置描述符 */
typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} uart_dma_config_t;

/* ---------- 中断传输状态 ---------- */
typedef struct {
    uint8_t    *buf;        /* 循环接收缓冲区 */
    uint16_t    buf_size;   /* 缓冲区总大小 */
    uint16_t    pos;        /* 实际写入区域大小 */
} uart_cache_t;
typedef struct {
    uart_cache_t *tx_user_buf;
    uart_cache_t *rx_user_buf;
    volatile bool  tx_user_active;
    volatile bool  rx_user_active;      /* dev_read 是否在等待 */
    Semaphore * uart_tx_sem;
    Semaphore * uart_rx_sem;
} uart_xfer_t;

/* USART 引脚描述 */
typedef struct {
    pin_af uart_tx;
    pin_af uart_rx;
} uart_pins_t;
typedef struct {
    uart_id_t id;
    uint32_t    baudrate;
    uart_word_len_t     word_len;
    uart_stop_t     stop_bits;
    uart_parity_t     parity;
    uart_pins_t pins;
    uart_it_t it_enable;
    uint16_t cache_size;
    /* DMA 可选 */
    const uart_dma_config_t *dma_cfg;   /* 为 NULL 则表示不使用 DMA */
} usart_config_t;

struct _Usart {
    Device base;  // 基类作为第一个成员
    const UsartFun* fun;
    RingBuf *rx_cache_buf;
    uart_xfer_t *uart_xfer;
    Semaphore     *uart_tx_sem;          /* 发送完成信号量 (listener 使用) */
    Semaphore     *uart_rx_sem;          /* 接收完成信号量 (listener 使用) */
};

// 构造函数声明
Usart* usart_create(const device_info_t *info);
void usart_init(Usart* self, const device_info_t *info);

// 析构函数声明
void usart_deinit(Usart* self);

#endif // USART_H
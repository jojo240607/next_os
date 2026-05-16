#ifndef USART_H
#define USART_H
#include <stdint.h>
#include <stdbool.h>
#include "common/device.h"
#include "common/pinmux.h"
#include "common/dma.h"
#include "hal/hal_usart.h"

#define GET_USART(obj) ((Usart *)obj)
#define DEFAULT_RX_BUFFER (256)
// 派生类声明
typedef struct _Usart Usart;
typedef struct _UsartFun UsartFun;
typedef struct _usart_config usart_config;
// 类成员函数结构
struct _UsartFun {
    void (*destroy)(Usart* self);
	void (*send_it)(Usart* self, const uint8_t *data, uint16_t len);

	void (*recv_it)(Usart* self, uint8_t *buffer, uint16_t len);

};


/* 字长 */
typedef enum : uint8_t {
    UART_WORDLEN_8  = 0x00,
    UART_WORDLEN_9  = 0x01,
} uart_word_len_t;


/* 停止位 */
typedef enum : uint8_t {
    UART_STOP_1     = 0x00,
    UART_STOP_0_5   = 0x01,
    UART_STOP_2     = 0x02,
    UART_STOP_1_5   = 0x03,
} uart_stop_t;
/* 校验 */
typedef enum : uint8_t {
    UART_PARITY_NONE  = 0x00,
    UART_PARITY_EVEN  = 0x02,
    UART_PARITY_ODD   = 0x03,
} uart_parity_t;

typedef enum : uint8_t {
    /* 中断使能选项 */
    xUART_IT_NONE = 0,
    xUART_IT_TXE =  (1 << 0),   // 发送数据寄存器空中断
    xUART_IT_RXNE = (1 << 1),   // 接收数据寄存器非空中断
    xUART_IT_TC =   (1 << 2),   // 发送完成中断 (TC)
} uart_it_t;
/* UART DMA 配置描述符 */
typedef struct {
    const dma_stream_config_t *tx_dma;
    const dma_stream_config_t *rx_dma;
} uart_dma_config_t;

/* ---------- 中断传输状态 ---------- */
typedef struct {
    const uint8_t *tx_buf;
    uint8_t       *rx_buf;
    uint16_t       tx_total;
    uint16_t       tx_index;
    uint16_t       rx_total;
    uint16_t       rx_index;
    bool           tx_active;
    bool           rx_active;
} uart_xfer_t;

struct _usart_config {
    uart_id_t id;
    uint32_t    baudrate;
    uart_word_len_t     word_len;
    uart_stop_t     stop_bits;
    uart_parity_t     parity;
    const pin_config_t *tx_conf;
    const pin_config_t *rx_conf;
    uart_it_t it_enable;
    /* DMA 可选 */
    const uart_dma_config_t *dma_cfg;   /* 为 NULL 则表示不使用 DMA */
};

struct _Usart {
    Device base;  // 基类作为第一个成员
    const UsartFun* fun;
    // TODO: 添加派生类特有的数据成员
    const usart_config *conf;
    uart_xfer_t uart_xfer;
    Semaphore * uart_tx_sem;
    Semaphore * uart_rx_sem;
};

// 构造函数声明
Usart* usart_create(const usart_config * conf);
void usart_init(Usart* self, const usart_config *conf);

// 析构函数声明
void usart_deinit(Usart* self);

#endif // USART_H
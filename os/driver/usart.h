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
    void (*send)(Usart* self, const uint8_t *data, uint16_t len);
    void (*recv)(Usart* self, uint8_t *buffer, uint16_t len);

};



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
/* USART 引脚描述 */
typedef struct {
    pin_af uart_tx;
    pin_af uart_rx;
} uart_pins_t;
struct _usart_config {
    uart_id_t id;
    uint32_t    baudrate;
    uart_word_len_t     word_len;
    uart_stop_t     stop_bits;
    uart_parity_t     parity;
    uart_pins_t pins;
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
    volatile Semaphore * uart_tx_sem;
    volatile Semaphore * uart_rx_sem;
};

// 构造函数声明
Usart* usart_create(const usart_config * conf, const dev_pripority_t *priority);
void usart_init(Usart* self, const usart_config *conf, const dev_pripority_t *priority);

// 析构函数声明
void usart_deinit(Usart* self);

#endif // USART_H
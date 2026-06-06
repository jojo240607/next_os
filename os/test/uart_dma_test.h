#ifndef UART_DMA_TEST_H
#define UART_DMA_TEST_H
#include <stdint.h>
#include <stdbool.h>
#include "../task/task.h"
#include "../driver/usart.h"

#define GET_UART_DMA_TEST_VTABLE(obj) GET_TASK_VTABLE(obj)
#define GET_UART_DMA_TEST(obj) ((UartDmaTest *)obj)

/* 派生类声明 */
typedef struct _UartDmaTest UartDmaTest;
typedef struct _UartDmaTestFun UartDmaTestFun;

struct _UartDmaTestFun {
    void (*destroy)(UartDmaTest* self);
};

struct _UartDmaTest {
    Task base;
    const UartDmaTestFun* fun;
    Device *usart4;          /* USART4 设备句柄 */
    uint32_t send_count;     /* 发送计数 */
    uint32_t recv_count;     /* 接收计数 */
    char tx_buffer[128];
    char rx_buffer[256];
};

/* 构造函数声明 */
UartDmaTest* uart_dma_test_create(const task_into_t *info);
void uart_dma_test_init(UartDmaTest* self, const task_into_t *info);

/* 析构函数声明 */
void uart_dma_test_deinit(UartDmaTest* self);

#endif /* UART_DMA_TEST_H */

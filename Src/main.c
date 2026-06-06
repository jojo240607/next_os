/**
 * main.c - 项目入口
 *
 * 当前集成的测试功能：
 *   1. DMA + USART1 发送测试（uart_dma_test 任务）
 *      - 使用 DMA2 Stream7 Channel4 发送
 *      - 每秒发送一次 "Hello" 消息
 *      - USART1 TX: PA9, RX: PA10, 115200 8N1
 *   2. 其他任务（timer, test1, systick 等）正常调度
 */
#include <stdio.h>
#include "../os/common/os_init.h"


int main(void)
{
    os_init();
    while (1)
    {
    }
  return 0;  // 永远不会执行到这里
}



#ifndef USART_H
#define USART_H
#include <stdint.h>
#include <stdbool.h>
#include "Device.h"
#include "stm32f407xx.h"

#define GET_USART(obj) ((Usart *)obj)
#define DEFAULT_RX_BUFFER (256)
// 派生类声明
typedef struct _Usart Usart;
typedef struct _UsartFun UsartFun;
typedef struct _usart_config usart_config;
// 类成员函数结构
struct _UsartFun {
    void (*destroy)(Usart* self);
};

struct _usart_config {
    GPIO_TypeDef *gpio_type;
    USART_TypeDef *usart_type;
    uint32_t bound;
    bool feedback;
    uint16_t buffer_size;
    irq_config irq_conf;
};

struct _Usart {
    Device base;  // 基类作为第一个成员
    const UsartFun* fun;
    // TODO: 添加派生类特有的数据成员
    // 假设的全局变量，用于存储接收到的数据
    usart_config *conf;
    volatile uint8_t rx_index;
    volatile uint8_t rx_complete;
    uint16_t rx_size;
    uint8_t rx_buffer[];
   //bool feedback;
};

// 构造函数声明
Usart* usart_create(usart_config * conf);
void usart_init(Usart* self, usart_config *conf);

// 析构函数声明
void usart_deinit(Usart* self);
bool usart_irq_handler_impl(void *arg);
#endif // USART_H
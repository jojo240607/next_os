#ifndef USART_H
#define USART_H
#include <stdint.h>
#include <stdbool.h>
#include "Device.h"

#define GET_USART(obj) ((Usart *)obj)

// 派生类声明
typedef struct _Usart Usart;
typedef struct _UsartFun UsartFun;
// 类成员函数结构
struct _UsartFun {
    void (*destroy)(Usart* self);
};
struct _Usart {
    Device base;  // 基类作为第一个成员
    const UsartFun* fun;
    // TODO: 添加派生类特有的数据成员
};

// 构造函数声明
Usart* usart_create();
void usart_init(Usart* self);

// 析构函数声明
void usart_deinit(Usart* self);

#endif // USART_H
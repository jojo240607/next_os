#include "usart.h"
#include "../common/linear_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "../log/log.h"


dev_init_override(usart_dev_init_impl);
dev_read_override(usart_dev_read_impl);
dev_write_override(usart_dev_write_impl);
dev_ioctl_override(usart_dev_ioctl_impl);

// 析构函数声明
static void usart_destroy(Usart* self);

// TODO: 初始化数据成员
static const UsartFun usart_fun = {
    .destroy = usart_destroy,
};
// 构造函数实现
Usart* usart_create(usart_config * conf) {
    if (conf == NULL) {
        return NULL;
    }
    Usart* obj = (Usart*)os_malloc(sizeof(Usart) + conf->buffer_size == 0 ? DEFAULT_RX_BUFFER : conf->buffer_size);
    if (obj) {
        memset(obj, 0, sizeof(Usart) + conf->buffer_size == 0 ? DEFAULT_RX_BUFFER : conf->buffer_size);
        usart_init(obj, conf);
    }
    return obj;
}

void usart_init(Usart* self, usart_config * conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(usart_fun);
    // TODO: 初始化派生类特有成员

    GET_DEVICE_VTABLE(self)->dev_init = usart_dev_init_impl;
    GET_DEVICE_VTABLE(self)->dev_read = usart_dev_read_impl;
    GET_DEVICE_VTABLE(self)->dev_write = usart_dev_write_impl;
    GET_DEVICE_VTABLE(self)->dev_ioctl = usart_dev_ioctl_impl;
    self->rx_complete = 0;
    self->rx_index = 0;
    self->conf = conf;
    self->rx_size = conf->buffer_size == 0 ? DEFAULT_RX_BUFFER : conf->buffer_size;
}

void usart_deinit(Usart* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void usart_destroy(Usart* self) {
    if (self != NULL) {
        usart_deinit(self);
        os_free(self);
    }
}

// dev_init method Semaphore *sem, intc_handler_t handle, void* arg
dev_init_override(usart_dev_init_impl) {
    // TODO: add dev_init method
    Usart *usart = (Usart *)self;
    //params
    // 使能UART4时钟 (APB1总线，位19)
    RCC->APB1ENR |= (1 << 19);
// 使能GPIOC时钟 (AHB1总线，位2)
    RCC->AHB1ENR |= (1 << 2);

    // 1. 将PC10和PC11配置为复用功能
    usart->conf->gpio_type->MODER &= ~(0xF << (10*2)); // 清除原有的模式位
    usart->conf->gpio_type->MODER |=  (0x2 << (10*2)); // 10: Alternate Function mode
    usart->conf->gpio_type->MODER |=  (0x2 << (11*2));

// 2. 将PC10（TX）配置为推挽输出
    usart->conf->gpio_type->OTYPER &= ~(1 << 10);      // 0: Output push-pull
// 3. 设置输出速度为50MHz
    usart->conf->gpio_type->OSPEEDR |= (0x2 << (10*2)); // 10: 50MHz
// 4. 配置无上拉/下拉
    usart->conf->gpio_type->PUPDR &= ~(0x3 << (10*2));
    usart->conf->gpio_type->PUPDR &= ~(0x3 << (11*2));

// 5. 配置复用功能为AF8 (UART4)
    usart->conf->gpio_type->AFR[1] &= ~(0xF << ((10-8)*4));
    usart->conf->gpio_type->AFR[1] |=  (0x8 << ((10-8)*4)); // AF8 for PC10
    usart->conf->gpio_type->AFR[1] &= ~(0xF << ((11-8)*4));
    usart->conf->gpio_type->AFR[1] |=  (0x8 << ((11-8)*4)); // AF8 for PC11


    // 1. 配置数据位8位 (M位0) 和 无校验 (PCE位0)
    usart->conf->usart_type->CR1 &= ~(1 << 12); // 清除PCE位: 无校验
    usart->conf->usart_type->CR1 &= ~(1 << 12); // 重复清除是为了保险, 确保PCE位为0
    usart->conf->usart_type->CR1 &= ~(1 << 13); // UE bit, 稍后使能
// 注意: M位在CR1的第12位, 但我们刚刚清除了PCE位, 这可能会影响M位。正确做法：
    usart->conf->usart_type->CR1 &= ~(1 << 12); // PCE=0
    usart->conf->usart_type->CR1 &= ~(1 << 12); // 确保PCE位清除

// 正确设置数据位8位 (M=0)
    usart->conf->usart_type->CR1 &= ~(1 << 12); // 清除M位 (注意: 如果PCE=1, M位在USART_CR1的第12位, 但这里我们PCE=0, 所以M位配置独立)
// 澄清: 在STM32F4中, CR1寄存器第12位是M位, 第10位是PCE位。
// 为了避免混淆，先清除M位和PCE位：
    usart->conf->usart_type->CR1 &= ~((1 << 12) | (1 << 10)); // M=0, 8位; PCE=0, 无校验

// 2. 配置停止位 (默认为1个停止位，CR2的第13、12位均为0)
    usart->conf->usart_type->CR2 &= ~(0x3 << 12); // 清除STOP位

// 3. 计算波特率 (假设系统时钟为84MHz, 目标波特率115200)
// BRR = 时钟频率 / 目标波特率 (当OVER8=0时)
    uint32_t brr_value = 84000000 / usart->conf->bound;
    usart->conf->usart_type->BRR = brr_value; // 整数部分直接写入
// 更精确的分数波特率生成公式：
// USARTDIV = 84MHz / (115200 * 16) = 45.5729...
// DIV_Mantissa = 45, DIV_Fraction = 16 * 0.5729 = 9.166 -> 9
// BRR = (45 << 4) | 9 = 729
    usart->conf->usart_type->BRR = 729; // 对应84MHz时钟下, 115200波特率
// 4. 使能发送器 (TE位=1) 和 接收器 (RE位=1)
    usart->conf->usart_type->CR1 |= (1 << 3) | (1 << 2); // 设置TE位和RE位

    if (usart->conf->irq_conf.handler != NULL) {
        // 5. 使能UART4外设 (UE位=1)
        usart->conf->usart_type->CR1 |= (1 << 13);
        // --- 中断配置 (例如使能接收中断) ---
        usart->conf->usart_type->CR1 |= (1 << 5); // 使能接收中断 (RXNEIE)
        if (!self->fun->attach_irq(self, &usart->conf->irq_conf, sem)) {
            LOG_ERROR("systick", "attach irq %d error", usart->conf->irq_conf.irq_num);
        }
    }

}
// dev_read method
dev_read_override(usart_dev_read_impl) {
    // TODO: add dev_read method
    Usart *usart = (Usart *)self;
    //params , void *buf, size_t count
    while(!(usart->conf->usart_type->SR & (1 << 5)));
    *(char *)buf = (char)usart->conf->usart_type->DR;
}
// dev_write method
dev_write_override(usart_dev_write_impl) {
    // TODO: add dev_write method
    Usart *usart = (Usart *)self;
    //params , const void *buf, size_t count
    // 检查发送数据寄存器是否为空 (TXE标志位)
    while (count--) {
        while (!(usart->conf->usart_type->SR & (1 << 7)));
        usart->conf->usart_type->DR = *(char *) buf++;
    }
}
// dev_ioctl method
dev_ioctl_override(usart_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    //Usart *usart = (Usart *)self;
    //params , int cmd, void *arg
    
}

// irq_handler method
bool usart_irq_handler_impl(void *arg) {
    // TODO: add irq_handler method
    Usart *usart = (Usart *)arg;
    //params , void *arg
    // 检查SR寄存器的RXNE位，表示接收到了新数据
    if (usart->conf->usart_type->SR & (1 << 5)) {
        char received_char = usart->conf->usart_type->DR; // 读取数据寄存器，硬件会自动清除RXNE标志
        if (usart->conf->feedback) {
            while (!(usart->conf->usart_type->SR & (1 << 7)));
            usart->conf->usart_type->DR = received_char;
        }
        // 将数据存入缓冲区
        if (usart->rx_index < usart->rx_size - 1) {
            usart->rx_buffer[usart->rx_index++] = received_char;
            if (received_char == '\n' || received_char == '\r') {
                usart->rx_buffer[usart->rx_index++] = '\n';
                usart->rx_buffer[usart->rx_index] = '\0'; // 字符串结束符
                usart->rx_complete = 1;
                usart->rx_index = 0;
                return true;//收到结束符，才触发信号量通知程序解析
            }
        }
    }
    return false;
}




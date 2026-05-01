//
// 驱动舒适化配置参数表
//
#include "device_config.h"


const systick_conf sys_tick_conf = {.systick_frequency = 168000000,
        {.irq_num = SYSTIC_IRQ,
         .handler = systick_irq_handler_impl,
         .priority = 0}};


const usart_config usart4_conf = {
        .gpio_type = GPIOC,
        .usart_type = UART4,
        .bound = 115200,
        .feedback = true,
        .buffer_size = 64,
        {.irq_num = USART4_IRQ,
         .handler = usart_irq_handler_impl,
         .priority = 0}};

const usart_config usart3_conf = {
        .gpio_type = GPIOC,
        .usart_type = USART3,
        .bound = 115200,
        .feedback = true,
        .buffer_size = 64,
        {.irq_num = USART3_IRQ,
         .handler = usart_irq_handler_impl,
         .priority = 0}};

const timer_config time2_conf = {
        .timer_type = TIM2,
        .time_frenqurncy = 500,
        {.irq_num = TIM2_IRQ,
         .handler = timer_irq_handler_impl,
         .priority = 1}
};

const exti_config exti_conf = {
        //.gpio_type = GPIOA,
        //.exti_line = 0x0022,//0000 0000 0010 0010   1和6线
        .pin_size = 2,
        .pin_conf = {{.port = PORT_A,
                      .irq_mode = PIN_IRQ_MODE_FALLING,
                      .pin = 1,
                      .mode = PIN_MODE_INPUT,
                      .ospeed = PIN_OSPEED_HIGH,
                      .otype = PIN_OTYPE_OD,
                      .pupd = PIN_PUPD_PULLUP,
                      .af = 0},
                     {.port = PORT_B,
                       .irq_mode = PIN_IRQ_MODE_FALLING,
                       .pin = 6,
                       .mode = PIN_MODE_INPUT,
                       .ospeed = PIN_OSPEED_HIGH,
                       .otype = PIN_OTYPE_OD,
                       .pupd = PIN_PUPD_PULLUP,
                       .af = 0}}
};
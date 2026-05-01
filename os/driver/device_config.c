//
// 驱动舒适化配置参数表
//
#include "device_config.h"


const systick_conf sys_tick_conf = {.systick_frequency = 168000000,
        {.irq_num = SYSTIC_IRQ,
         .handler = systick_irq_handler_impl,
         .priority = 0}};


const usart_config usart1_conf = {
        .usart_id = UART_1,
        .bound = 115200,
        .feedback = true,
        .buffer_size = 64,
        {.irq_num = USART1_IRQ,
         .handler = usart_irq_handler_impl,
         .priority = 0},
         .pin_conf = {
                {.port = PORT_A,
                        .port = 9,
                        .mode = PIN_MODE_AF,
                        .otype  = PIN_OTYPE_PP,
                        .ospeed = PIN_OSPEED_HIGH,
                        .pupd   = PIN_PUPD_PULLUP,
                        .af     = 7            /* USART1_TX 使用 AF7 */},
                {.port   = PORT_A,
                        .pin    = 10,
                        .mode   = PIN_MODE_AF,
                        .otype  = PIN_OTYPE_PP,
                        .ospeed = PIN_OSPEED_HIGH,
                        .pupd   = PIN_PUPD_PULLUP,
                        .af     = 7            /* USART1_RX */}}};

const usart_config usart4_conf = {
        .usart_id = UART_4,
        .bound = 115200,
        .feedback = true,
        .buffer_size = 64,
        {.irq_num = USART4_IRQ,
         .handler = usart_irq_handler_impl,
         .priority = 0},
         .pin_size = 2,
         .pin_conf = {
                 {.port = PORT_C,
                         .pin = 10,
                         .mode = PIN_MODE_AF,
                         .ospeed = PIN_OSPEED_HIGH,
                         .otype = PIN_OTYPE_PP,
                         .pupd = PIN_PUPD_PULLUP,
                         .af = 8,//配置复用功能为AF8 (UART4)
                         .irq_mode = PIN_IRQ_MODE_NONE},
                 {.port = PORT_C,
                         .pin = 11,
                         .mode = PIN_MODE_AF,
                         .ospeed = PIN_OSPEED_HIGH,
                         .otype = PIN_OTYPE_PP,
                         .pupd = PIN_PUPD_PULLUP,
                         .af = 8,//配置复用功能为AF8 (UART4)
                         .irq_mode = PIN_IRQ_MODE_NONE}
        }};

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
                       .af = 0}
        }
};
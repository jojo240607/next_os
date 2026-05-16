//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_DMA_H
#define STM32F4DISCOVERY_HAL_DMA_H
#include "stdint.h"

/* 编码公式：((ctrl)<<8) | ((stream)<<4) | (channel) */
#define  DMA_REQ_ENCODE(ctrl, stream, ch)  (((ctrl) << 8) | ((stream) << 4) | (ch))

/* 解码宏 */
#define DMA_REQ_GET_CTRL(req)   ((req) >> 8 & 0x0F)
#define DMA_REQ_GET_STREAM(req) (((req) >> 4) & 0x0F)
#define DMA_REQ_GET_CHANNEL(req) ((req) & 0x0F)
/* DMA 控制器编号 (用于编码) */
/* 控制器选择 */
typedef enum : uint8_t {
    DMA_1 = 0,
    DMA_2 = 1,
    xDMA_CONTROLLER_MAX
} dma_controller_t;

typedef enum : uint16_t {
    /* ================= DMA1 请求 (编码: (1<<8) | (Stream<<4) | Channel) ================= */
    /* Channel 0 */
    DMA1_REQ_SPI3_RX_ST0     = DMA_REQ_ENCODE(DMA_1, 0, 0),
    DMA1_REQ_SPI3_RX_ST2     = DMA_REQ_ENCODE(DMA_1, 2, 0),
    DMA1_REQ_SPI2_RX         = DMA_REQ_ENCODE(DMA_1, 3, 0),   // 仅 Stream3
    DMA1_REQ_SPI2_TX         = DMA_REQ_ENCODE(DMA_1, 4, 0),   // 仅 Stream4
    DMA1_REQ_SPI3_TX_ST5     = DMA_REQ_ENCODE(DMA_1, 5, 0),   // 仅 Stream5
    DMA1_REQ_SPI3_TX_ST7     = DMA_REQ_ENCODE(DMA_1, 7, 0),   // 仅 Stream7

    /* Channel 1 */
    DMA1_REQ_I2C1_RX_ST0     = DMA_REQ_ENCODE(DMA_1, 0, 1),
    DMA1_REQ_TIM7_UP_ST2     = DMA_REQ_ENCODE(DMA_1, 2, 1),
    DMA1_REQ_TIM7_UP_ST4     = DMA_REQ_ENCODE(DMA_1, 4, 1),
    DMA1_REQ_I2C1_RX_ST5     = DMA_REQ_ENCODE(DMA_1, 5, 1),
    DMA1_REQ_I2C1_TX_ST6     = DMA_REQ_ENCODE(DMA_1, 6, 1),
    DMA1_REQ_I2C1_TX_ST7     = DMA_REQ_ENCODE(DMA_1, 7, 1),

    /* Channel 2 */
    DMA1_REQ_TIM4_CH1        = DMA_REQ_ENCODE(DMA_1, 0, 2),   // 仅 Stream0
    DMA1_REQ_I2S3_RX_ST2     = DMA_REQ_ENCODE(DMA_1, 2, 2),
    DMA1_REQ_TIM4_CH2        = DMA_REQ_ENCODE(DMA_1, 3, 2),   // 仅 Stream3
    DMA1_REQ_I2S2_TX         = DMA_REQ_ENCODE(DMA_1, 4, 2),
    DMA1_REQ_I2S3_TX         = DMA_REQ_ENCODE(DMA_1, 5, 2),
    DMA1_REQ_TIM4_UP         = DMA_REQ_ENCODE(DMA_1, 6, 2),   // 仅 Stream6
    DMA1_REQ_TIM4_CH3        = DMA_REQ_ENCODE(DMA_1, 7, 2),   // 仅 Stream7

    /* Channel 3 */
    DMA1_REQ_I2S3_RX_ST0     = DMA_REQ_ENCODE(DMA_1, 0, 3),
    DMA1_REQ_TIM2_CH3        = DMA_REQ_ENCODE(DMA_1, 1, 3),
    DMA1_REQ_TIM2_UP_ST1         = DMA_REQ_ENCODE(DMA_1, 1, 3),
    DMA1_REQ_I2C3_RX         = DMA_REQ_ENCODE(DMA_1, 2, 3),
    DMA1_REQ_I2S2_RX         = DMA_REQ_ENCODE(DMA_1, 3, 3),
    DMA1_REQ_I2C3_TX         = DMA_REQ_ENCODE(DMA_1, 4, 3),
    DMA1_REQ_TIM2_CH1        = DMA_REQ_ENCODE(DMA_1, 5, 3),
    DMA1_REQ_TIM2_CH2        = DMA_REQ_ENCODE(DMA_1, 6, 3),
    DMA1_REQ_TIM2_CH4_ST6    = DMA_REQ_ENCODE(DMA_1, 6, 3),   // 与 CH2 共用 Stream6
    DMA1_REQ_TIM2_UP_ST7     = DMA_REQ_ENCODE(DMA_1, 7, 3),
    DMA1_REQ_TIM2_CH4_ST7    = DMA_REQ_ENCODE(DMA_1, 7, 3),

    /* Channel 4 */
    DMA1_REQ_UART5_RX        = DMA_REQ_ENCODE(DMA_1, 0, 4),
    DMA1_REQ_USART3_RX       = DMA_REQ_ENCODE(DMA_1, 1, 4),
    DMA1_REQ_UART4_RX        = DMA_REQ_ENCODE(DMA_1, 2, 4),
    DMA1_REQ_USART3_TX_ST3       = DMA_REQ_ENCODE(DMA_1, 3, 4),
    DMA1_REQ_UART4_TX        = DMA_REQ_ENCODE(DMA_1, 4, 4),
    DMA1_REQ_USART2_RX       = DMA_REQ_ENCODE(DMA_1, 5, 4),
    DMA1_REQ_USART2_TX       = DMA_REQ_ENCODE(DMA_1, 6, 4),
    DMA1_REQ_UART5_TX        = DMA_REQ_ENCODE(DMA_1, 7, 4),


    /* Channel 5 */
    //DMA1_REQ_UART8_TX        = DMA_REQ_ENCODE(DMA_1, 0, 5),
    //DMA1_REQ_UART7_TX        = DMA_REQ_ENCODE(DMA_1, 1, 5),
    DMA1_REQ_TIM3_UP         = DMA_REQ_ENCODE(DMA_1, 2, 5),
    DMA1_REQ_TIM3_CH4         = DMA_REQ_ENCODE(DMA_1, 2, 5),
    //DMA1_REQ_UART7_RX        = DMA_REQ_ENCODE(DMA_1, 3, 5),
    DMA1_REQ_TIM3_CH1        = DMA_REQ_ENCODE(DMA_1, 4, 5),
    DMA1_REQ_TIM3_TRIG       = DMA_REQ_ENCODE(DMA_1, 4, 5),   // 与 CH1 共用 Stream4
    DMA1_REQ_TIM3_CH2        = DMA_REQ_ENCODE(DMA_1, 5, 5),
    //DMA1_REQ_UART8_RX        = DMA_REQ_ENCODE(DMA_1, 6, 5),
    DMA1_REQ_TIM3_CH3        = DMA_REQ_ENCODE(DMA_1, 7, 5),

    /* Channel 6 */
    DMA1_REQ_TIM5_CH3        = DMA_REQ_ENCODE(DMA_1, 0, 6),
    DMA1_REQ_TIM5_UP_ST0     = DMA_REQ_ENCODE(DMA_1, 0, 6),
    DMA1_REQ_TIM5_CH4_ST1    = DMA_REQ_ENCODE(DMA_1, 1, 6),
    DMA1_REQ_TIM5_TRIG_ST1   = DMA_REQ_ENCODE(DMA_1, 1, 6),   // 与 CH4 共用 Stream1
    DMA1_REQ_TIM5_CH1        = DMA_REQ_ENCODE(DMA_1, 2, 6),
    DMA1_REQ_TIM5_CH4_ST3    = DMA_REQ_ENCODE(DMA_1, 3, 6),
    DMA1_REQ_TIM5_TRIG_ST3   = DMA_REQ_ENCODE(DMA_1, 3, 6),
    DMA1_REQ_TIM5_CH2        = DMA_REQ_ENCODE(DMA_1, 4, 6),
    DMA1_REQ_TIM5_UP_ST6     = DMA_REQ_ENCODE(DMA_1, 6, 6),


    /* Channel 7 */
    DMA1_REQ_TIM6_UP         = DMA_REQ_ENCODE(DMA_1, 1, 7),
    DMA1_REQ_I2C2_RX_ST2     = DMA_REQ_ENCODE(DMA_1, 2, 7),
    DMA1_REQ_I2C2_RX_ST3     = DMA_REQ_ENCODE(DMA_1, 3, 7),
    DMA1_REQ_USART3_TX_ST4   = DMA_REQ_ENCODE(DMA_1, 4, 7),
    DMA1_REQ_DAC1            = DMA_REQ_ENCODE(DMA_1, 5, 7),
    DMA1_REQ_DAC2            = DMA_REQ_ENCODE(DMA_1, 6, 7),
    DMA1_REQ_I2C2_TX         = DMA_REQ_ENCODE(DMA_1, 7, 7),

    /* ======================== DMA2 请求 ========================= */
    /* Channel 0 */
    DMA2_REQ_ADC1_ST0            = DMA_REQ_ENCODE(DMA_2, 0, 0),
    //DMA2_REQ_SAI1_A_ST1        = DMA_REQ_ENCODE(DMA_2, 1, 0),
    DMA2_REQ_TIM8_CH1_ST2_CN0    = DMA_REQ_ENCODE(DMA_2, 2, 0),
    DMA2_REQ_TIM8_CH2_ST2        = DMA_REQ_ENCODE(DMA_2, 2, 0),
    DMA2_REQ_TIM8_CH3_ST2        = DMA_REQ_ENCODE(DMA_2, 2, 0),
    //DMA2_REQ_SAI1_A_ST3        = DMA_REQ_ENCODE(DMA_2, 3, 0),
    DMA2_REQ_ADC1_ST4            = DMA_REQ_ENCODE(DMA_2, 4, 0),
    //DMA2_REQ_SAI1_B            = DMA_REQ_ENCODE(DMA_2, 5, 0),
    DMA2_REQ_TIM1_CH1_ST6        = DMA_REQ_ENCODE(DMA_2, 6, 0),
    DMA2_REQ_TIM1_CH2_ST6        = DMA_REQ_ENCODE(DMA_2, 6, 0),   // 与 CH1 共用
    DMA2_REQ_TIM1_CH3_ST6_CN0    = DMA_REQ_ENCODE(DMA_2, 6, 0),   // 共用 Stream6

    /* Channel 1 */
    DMA2_REQ_DCMI_ST1            = DMA_REQ_ENCODE(DMA_2, 1, 1),
    DMA2_REQ_ADC2_ST2            = DMA_REQ_ENCODE(DMA_2, 2, 1),
    DMA2_REQ_ADC2_ST3            = DMA_REQ_ENCODE(DMA_2, 3, 1),
    //DMA2_REQ_SAI1_B            = DMA_REQ_ENCODE(DMA_2, 4, 1),
    //DMA2_REQ_SPI6_TX           = DMA_REQ_ENCODE(DMA_2, 5, 1),
    //DMA2_REQ_SPI6_RX           = DMA_REQ_ENCODE(DMA_2, 6, 1),
    DMA2_REQ_DCMI_ST7            = DMA_REQ_ENCODE(DMA_2, 7, 1),


    /* Channel 2 */
    DMA2_REQ_ADC3_ST0            = DMA_REQ_ENCODE(DMA_2, 0, 2),
    DMA2_REQ_ADC3_ST1            = DMA_REQ_ENCODE(DMA_2, 1, 2),
    //DMA2_REQ_SPI5_RX           = DMA_REQ_ENCODE(DMA_2, 3, 2),
    //DMA2_REQ_SPI5_TX           = DMA_REQ_ENCODE(DMA_2, 4, 2),
    DMA2_REQ_CRYP_OUT            = DMA_REQ_ENCODE(DMA_2, 5, 2),
    DMA2_REQ_CRYP_IN             = DMA_REQ_ENCODE(DMA_2, 6, 2),
    DMA2_REQ_HASH_IN             = DMA_REQ_ENCODE(DMA_2, 7, 2),


    /* Channel 3 */
    DMA2_REQ_SPI1_RX_ST0         = DMA_REQ_ENCODE(DMA_2, 0, 3),
    DMA2_REQ_SPI1_RX_ST2         = DMA_REQ_ENCODE(DMA_2, 2, 3),
    DMA2_REQ_SPI1_TX_ST3         = DMA_REQ_ENCODE(DMA_2, 3, 3),
    DMA2_REQ_SPI1_TX_ST5         = DMA_REQ_ENCODE(DMA_2, 5, 3),

    /* Channel 4 */
    //DMA2_REQ_SPI4_RX           = DMA_REQ_ENCODE(DMA_2, 0, 4),
    //DMA2_REQ_SPI4_TX           = DMA_REQ_ENCODE(DMA_2, 1, 4),
    DMA2_REQ_USART1_RX_ST2       = DMA_REQ_ENCODE(DMA_2, 2, 4),
    DMA2_REQ_SDIO_ST3            = DMA_REQ_ENCODE(DMA_2, 3, 4),
    DMA2_REQ_USART1_RX_ST5       = DMA_REQ_ENCODE(DMA_2, 5, 4),
    DMA2_REQ_SDIO_ST6            = DMA_REQ_ENCODE(DMA_2, 6, 4),
    DMA2_REQ_USART1_TX           = DMA_REQ_ENCODE(DMA_2, 7, 4),

    /* Channel 5 */
    DMA2_REQ_USART6_RX_ST1       = DMA_REQ_ENCODE(DMA_2, 1, 5),
    DMA2_REQ_USART6_RX_ST2       = DMA_REQ_ENCODE(DMA_2, 2, 5),
    //DMA2_REQ_SPI4_RX           = DMA_REQ_ENCODE(DMA_2, 3, 5),
    //DMA2_REQ_SPI4_TX           = DMA_REQ_ENCODE(DMA_2, 4, 5),
    DMA2_REQ_USART6_TX_ST6       = DMA_REQ_ENCODE(DMA_2, 6, 5),
    DMA2_REQ_USART6_TX_ST7       = DMA_REQ_ENCODE(DMA_2, 7, 5),

    /* Channel 6 */
    DMA2_REQ_TIM1_TRIG_ST0       = DMA_REQ_ENCODE(DMA_2, 0, 6),
    DMA2_REQ_TIM1_CH1_ST1        = DMA_REQ_ENCODE(DMA_2, 1, 6),
    DMA2_REQ_TIM1_CH2_ST2        = DMA_REQ_ENCODE(DMA_2, 2, 6),
    DMA2_REQ_TIM1_CH1_ST3        = DMA_REQ_ENCODE(DMA_2, 3, 6),
    DMA2_REQ_TIM1_CH4            = DMA_REQ_ENCODE(DMA_2, 4, 6),
    DMA2_REQ_TIM1_TRIG_ST4       = DMA_REQ_ENCODE(DMA_2, 4, 6),
    DMA2_REQ_TIM1_COM            = DMA_REQ_ENCODE(DMA_2, 4, 6),
    DMA2_REQ_TIM1_UP             = DMA_REQ_ENCODE(DMA_2, 5, 6),
    DMA2_REQ_TIM1_CH3_ST6_CN6    = DMA_REQ_ENCODE(DMA_2, 6, 6),

    /* Channel 7 */
    DMA2_REQ_TIM8_UP             = DMA_REQ_ENCODE(DMA_2, 1, 7),
    DMA2_REQ_TIM8_CH1_ST2_CN7    = DMA_REQ_ENCODE(DMA_2, 2, 7),
    DMA2_REQ_TIM8_CH2_ST3        = DMA_REQ_ENCODE(DMA_2, 3, 7),
    DMA2_REQ_TIM8_CH3_ST4        = DMA_REQ_ENCODE(DMA_2, 4, 7),
    //DMA2_REQ_SPI5_RX           = DMA_REQ_ENCODE(DMA_2, 5, 7),
    //DMA2_REQ_SPI5_TX           = DMA_REQ_ENCODE(DMA_2, 6, 7),
    DMA2_REQ_TIM8_CH4            = DMA_REQ_ENCODE(DMA_2, 7, 7),
    DMA2_REQ_TIM8_TRIG           = DMA_REQ_ENCODE(DMA_2, 7, 7),
    DMA2_REQ_TIM8_COM            = DMA_REQ_ENCODE(DMA_2, 7, 7),
} dma_request_t;

/* ---------- DMA 寄存器定义 ---------- */
typedef struct {
    volatile uint32_t LISR;
    volatile uint32_t HISR;
    volatile uint32_t LIFCR;
    volatile uint32_t HIFCR;
} xDMA_Base_TypeDef;

typedef struct {
    volatile uint32_t SxCR;
    volatile uint32_t SxNDTR;
    volatile uint32_t SxPAR;
    volatile uint32_t SxM0AR;
    volatile uint32_t SxM1AR;
    volatile uint32_t SxFCR;
} xDMA_Stream_TypeDef;

#define xDMA1_BASE       0x40026000UL
#define xDMA2_BASE       0x40026400UL

#define DMA1_STREAM_BASE(i)  (xDMA1_BASE + 0x10 + (i) * 0x18)  /* Stream i 寄存器起始 */
#define DMA2_STREAM_BASE(i)  (xDMA2_BASE + 0x10 + (i) * 0x18)

void dma_clock_enable(dma_controller_t ctrl);

#endif //STM32F4DISCOVERY_HAL_DMA_H

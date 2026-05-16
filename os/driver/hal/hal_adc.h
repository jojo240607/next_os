//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_ADC_H
#define STM32F4DISCOVERY_HAL_ADC_H
#include "stdint.h"


#define xADC1_BASE 0x40012000UL
#define xADC2_BASE 0x40012100UL
#define xADC3_BASE 0x40012200UL



typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;   // 注入通道，本例忽略
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;    // 注入序列
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} xADC_TypeDef;

typedef enum : uint8_t {
    ADC_1 = 0,
    ADC_2,
    ADC_3,
    ADC_MAX
} adc_id_t;

extern xADC_TypeDef* const ADCx[];

void hal_adc_clock_enable(adc_id_t id);

#endif //STM32F4DISCOVERY_HAL_ADC_H

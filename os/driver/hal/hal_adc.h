//
// Created by zhiwei.gong on 2026/5/12.
//

#ifndef STM32F4DISCOVERY_HAL_ADC_H
#define STM32F4DISCOVERY_HAL_ADC_H

#include <stdbool.h>
#include "stdint.h"
#include "hal_gpio.h"


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
/* ADC 通道描述符 */
typedef struct {
    gpio_port_t port;
    gpio_pin_t    pin;
    uint8_t channel;        /* 0..18, 注意温度/Vref 等内部通道 */
    uint8_t sample_time;    /* ADC_SMP_xxx */
} adc_channel_cfg_t;

extern xADC_TypeDef* const ADCx[];

void hal_adc_clock_enable(adc_id_t id);
void hal_adc_set_resolution_align(adc_id_t id, uint8_t resolution, uint8_t align);
void hal_adc_set_sample_time(adc_id_t id, uint8_t channel, uint8_t sample_time);
void hal_adc_set_regular_sequence(adc_id_t id, uint8_t num_channels, const uint8_t *channels);
void hal_adc_enable(adc_id_t id);
void hal_adc_start_calibration(adc_id_t id);
void hal_adc_start_conversion(adc_id_t id);
void hal_adc_wait_eoc(adc_id_t id);
uint16_t hal_adc_read_dr(adc_id_t id);
void hal_adc_enable_dma(adc_id_t id, bool continuous);
uint32_t hal_adc_get_dr_addr(adc_id_t id);

#endif //STM32F4DISCOVERY_HAL_ADC_H

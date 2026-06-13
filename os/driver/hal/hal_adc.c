//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_adc.h"
#include "../common/rcc.h"

xADC_TypeDef* const ADCx[] = {
        (xADC_TypeDef*)xADC1_BASE,
        (xADC_TypeDef*)xADC2_BASE,
        (xADC_TypeDef*)xADC3_BASE
};

void hal_adc_clock_enable(adc_id_t id)
{
    switch (id) {
        case ADC_1:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_ADC1EN);
            break;
        case ADC_2:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_ADC2EN);
            break;
        case ADC_3:
            rcc_periph_clock_enable(RCC_BUS_APB2, xRCC_APB2ENR_ADC3EN);
            break;
        default:
            return;
    }
    __asm volatile ("dsb" ::: "memory");
}

void hal_adc_set_resolution_align(adc_id_t id, uint8_t resolution, uint8_t align) {
    xADC_TypeDef *adc = ADCx[id];
    uint32_t cr1 = adc->CR1;
    cr1 &= ~(3 << 24);
    cr1 |= (resolution & 0x3) << 24;
    if (align) cr1 |= (1 << 11);
    else       cr1 &= ~(1 << 11);
    adc->CR1 = cr1;
}

void hal_adc_set_sample_time(adc_id_t id, uint8_t channel, uint8_t sample_time) {
    xADC_TypeDef *adc = ADCx[id];
    if (channel < 10) {
        adc->SMPR2 &= ~(7 << (channel * 3));
        adc->SMPR2 |= (sample_time & 0x7) << (channel * 3);
    } else {
        channel -= 10;
        adc->SMPR1 &= ~(7 << (channel * 3));
        adc->SMPR1 |= (sample_time & 0x7) << (channel * 3);
    }
}

void hal_adc_set_regular_sequence(adc_id_t id, uint8_t num_channels, const uint8_t *channels) {
    xADC_TypeDef *adc = ADCx[id];
    uint32_t sqr1 = (num_channels - 1) << 20;
    uint32_t sqr2 = 0, sqr3 = 0;
    for (int i = 0; i < num_channels; i++) {
        uint8_t ch = channels[i];
        if (i < 6)          sqr3 |= (ch & 0x1F) << (i * 5);
        else if (i < 12)    sqr2 |= (ch & 0x1F) << ((i - 6) * 5);
        else                sqr1 |= (ch & 0x1F) << ((i - 12) * 5);
    }
    adc->SQR1 = sqr1;
    adc->SQR2 = sqr2;
    adc->SQR3 = sqr3;
}

void hal_adc_enable(adc_id_t id) {
    ADCx[id]->CR2 |= (1 << 0);   /* ADON */
    for (volatile uint32_t i = 0; i < 100000; i++);
}

void hal_adc_start_calibration(adc_id_t id) {
    xADC_TypeDef *adc = ADCx[id];
    adc->CR2 |= (1 << 3);   /* RSTCAL */
    while (adc->CR2 & (1 << 3));
    adc->CR2 |= (1 << 2);   /* CAL */
    while (adc->CR2 & (1 << 2));
}

void hal_adc_start_conversion(adc_id_t id) {
    ADCx[id]->CR2 |= (1 << 30);   /* SWSTART */
}

void hal_adc_wait_eoc(adc_id_t id) {
    while (!(ADCx[id]->SR & (1 << 1)));
}

uint16_t hal_adc_read_dr(adc_id_t id) {
    return (uint16_t)ADCx[id]->DR;
}

void hal_adc_enable_dma(adc_id_t id, bool continuous) {
    ADCx[id]->CR2 |= (1 << 8);   /* DMA */
    if (continuous) ADCx[id]->CR2 |= (1 << 1);  /* CONT */
}

uint32_t hal_adc_get_dr_addr(adc_id_t id) {
    return (uint32_t)&ADCx[id]->DR;
}

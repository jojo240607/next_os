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

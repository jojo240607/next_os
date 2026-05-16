//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_i2c.h"
#include "../common/rcc.h"

xI2C_TypeDef* const I2Cx[] = {
        (xI2C_TypeDef*)xI2C1_BASE,
        (xI2C_TypeDef*)xI2C2_BASE,
        (xI2C_TypeDef*)xI2C3_BASE
};

/* ---------- 时钟与频率配置 ---------- */
void hal_i2c_clock_enable(i2c_id_t id)
{
    switch (id) {
        case I2C_1:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2C1EN);
            break;
        case I2C_2:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2C2EN);
            break;
        case I2C_3:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2C3EN);
            break;
        default:
            return;
    }
    __asm volatile ("dsb" ::: "memory");
}

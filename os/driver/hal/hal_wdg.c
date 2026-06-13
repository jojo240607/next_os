/**
 * hal_wdg.c — 看门狗 HAL 层（纯寄存器操作）
 * 所有 xIWDG / xWWDG / xRCC_CSR 寄存器访问统一在此文件中。
 */
#include "hal_wdg.h"
#include "cmsis_gcc.h"
/* ═══════════════════════════════════ IWDG ═══════════════════════════════════ */

void hal_iwdg_unlock(void)
{
    xIWDG->KR = xIWDG_KEY_UNLOCK;
}

void hal_iwdg_lock(void)
{
    xIWDG->KR = xIWDG_KEY_LOCK;
}

void hal_iwdg_set_prescaler(uint8_t prescaler)
{
    xIWDG->PR = prescaler & 0x07;
}

void hal_iwdg_set_reload(uint16_t reload)
{
    xIWDG->RLR = reload & 0x0FFF;
}

void hal_iwdg_wait_ready(void)
{
    while (xIWDG->SR & (1 << 0)) __NOP();   // PVU (Prescaler Value Update)
    while (xIWDG->SR & (1 << 1)) __NOP();   // RVU (Reload Value Update)
}

void hal_iwdg_enable(void)
{
    xIWDG->KR = xIWDG_KEY_ENABLE;
}

void hal_iwdg_reload(void)
{
    xIWDG->KR = xIWDG_KEY_RELOAD;
}

bool hal_iwdg_is_reset_flag(void)
{
    return (xRCC_CSR->CSR & xRCC_CSR_IWDGRSTF) != 0;
}

void hal_iwdg_clear_reset_flag(void)
{
    xRCC_CSR->CSR |= xRCC_CSR_RMVF;
}

bool hal_wwdg_is_reset_flag(void)
{
    return (xRCC_CSR->CSR & xRCC_CSR_WWDGRSTF) != 0;
}

/* ═══════════════════════════════════ WWDG ═══════════════════════════════════ */

void hal_wwdg_set_window(uint8_t window)
{
    uint32_t tmp = xWWDG->CFR & ~0x7F;
    xWWDG->CFR = tmp | (window & 0x7F);
}

void hal_wwdg_set_prescaler(uint8_t prescaler)
{
    uint32_t tmp = xWWDG->CFR & ~(0x03 << 7);
    xWWDG->CFR = tmp | ((prescaler & 0x03) << 7);
}

void hal_wwdg_enable_ewi(void)
{
    xWWDG->CFR |= (1 << 9);
}

void hal_wwdg_clear_ewi_flag(void)
{
    xWWDG->SR = 0;
}

void hal_wwdg_set_counter(uint8_t counter)
{
    xWWDG->CR = (counter & 0x7F) | (1 << 7);  // WDGA=1 启动
}

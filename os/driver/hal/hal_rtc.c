//
// Created by zhiwei.gong on 2026/5/14.
//

#include "hal_rtc.h"
#include "../common/rcc.h"

/* ---------- 辅助：解锁/锁定 RTC 写保护 ---------- */
void rtc_unlock(void)
{
    xRTC->WPR = xRTC_WPR_KEY1;
    xRTC->WPR = xRTC_WPR_KEY2;
}

void rtc_lock(void)
{
    xRTC->WPR = 0xFF;
}

/* ---------- 辅助：进入/退出初始化模式 ---------- */
int rtc_enter_init_mode(void)
{
    uint32_t timeout = 100000;
    xRTC->ISR |= xRTC_ISR_INIT;
    while (!(xRTC->ISR & xRTC_ISR_INITF)) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

void rtc_exit_init_mode(void)
{
    xRTC->ISR &= ~xRTC_ISR_INIT;
}

/* ---------- 辅助：等待同步 ---------- */
void rtc_wait_sync(void)
{
    while (!(xRTC->ISR & xRTC_ISR_RSF));
}

/* ---------- 辅助：使能备份域访问和时钟 ---------- */
int rtc_enable_backup_domain(void)
{
    // 1. 使能 PWR 时钟 (APB1 bit28)
    rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_PWREN);
    // 2. 使能备份域访问
    xPWR->CR |= xPWR_CR_DBP;
    // 3. 配置 RTC 时钟源
    rtc_unlock();
    return 0;
}

int rtc_select_clock_source(rtc_clk_src_t src)
{
    uint32_t bdcr = xRCC_BDCR->BDCR;

    switch (src) {
        case RTC_CLK_LSE:
            // 使能 LSE
            bdcr |= xRCC_BDCR_LSEON;
            xRCC_BDCR->BDCR = bdcr;
            // 等待 LSE 就绪
            {
                uint32_t timeout = 1000000;
                while (!(xRCC_BDCR->BDCR & xRCC_BDCR_LSERDY)) {
                    if (--timeout == 0) return -1;
                }
            }
            // 选择 LSE 作为 RTC 时钟
            bdcr = xRCC_BDCR->BDCR;
            bdcr &= ~xRCC_BDCR_RTCSEL_MASK;
            bdcr |= (0x01 << xRCC_BDCR_RTCSEL_SHIFT);  // LSE
            xRCC_BDCR->BDCR = bdcr;
            break;
        case RTC_CLK_LSI:
            // 使能 LSI
        {
            volatile uint32_t *CSR = (uint32_t *)0x40023874UL;
            *CSR |= (1 << 0);  // LSION
            uint32_t timeout = 1000000;
            while (!(*CSR & (1 << 1))) { if (--timeout == 0) return -1; }
        }
            bdcr = xRCC_BDCR->BDCR;
            bdcr &= ~xRCC_BDCR_RTCSEL_MASK;
            bdcr |= (0x02 << xRCC_BDCR_RTCSEL_SHIFT);  // LSI
            xRCC_BDCR->BDCR = bdcr;
            break;
        case RTC_CLK_HSE_DIV:
            // HSE 分频作为 RTC 时钟 (需先使能 HSE)
            bdcr = xRCC_BDCR->BDCR;
            bdcr &= ~xRCC_BDCR_RTCSEL_MASK;
            bdcr |= (0x03 << xRCC_BDCR_RTCSEL_SHIFT);  // HSE
            xRCC_BDCR->BDCR = bdcr;
            break;
        default:
            return -1;
    }

    // 使能 RTC 时钟
    xRCC_BDCR->BDCR |= xRCC_BDCR_RTCEN;
    return 0;
}

void hal_rtc_deinit(void)
{
    rtc_unlock();
    // 关闭所有中断
    xRTC->CR &= ~(xRTC_CR_ALRAIE | xRTC_CR_ALRBIE | xRTC_CR_WUTIE | xRTC_CR_TSIE);
    rtc_lock();
}

/* ===================================================================
   日历读写
   =================================================================== */
int rtc_set_time(const rtc_time_t *time)
{
    if (!time) return -1;
    rtc_unlock();
    if (rtc_enter_init_mode() != 0) { rtc_lock(); return -1; }
    // 写入时间 (BCD)
    xRTC->TR = ((time->hours   / 10) << 20) | ((time->hours   % 10) << 16) |
              ((time->minutes / 10) << 12) | ((time->minutes % 10) << 8)  |
              ((time->seconds / 10) << 4)  |  (time->seconds % 10);
    rtc_exit_init_mode();
    rtc_wait_sync();
    rtc_lock();
    return 0;
}

int rtc_set_date(const rtc_date_t *date)
{
    if (!date) return -1;
    rtc_unlock();
    if (rtc_enter_init_mode() != 0) { rtc_lock(); return -1; }
    // 写入日期 (BCD)
    xRTC->DR = ((date->year    / 10) << 20) | ((date->year    % 10) << 16) |
              ((date->month   / 10) << 12) | ((date->month   % 10) << 8)  |
              ((date->day     / 10) << 4)  |  (date->day     % 10);
    rtc_exit_init_mode();
    rtc_wait_sync();
    rtc_lock();
    return 0;
}

int rtc_get_time(rtc_time_t *time)
{
    if (!time) return -1;
    rtc_wait_sync();
    uint32_t tr = xRTC->TR;
    time->hours   = ((tr >> 20) & 0x3) * 10 + ((tr >> 16) & 0xF);
    time->minutes = ((tr >> 12) & 0x7) * 10 + ((tr >> 8) & 0xF);
    time->seconds = ((tr >> 4)  & 0x7) * 10 + ((tr >> 0) & 0xF);
    return 0;
}

int rtc_get_date(rtc_date_t *date)
{
    if (!date) return -1;
    rtc_wait_sync();
    uint32_t dr = xRTC->DR;
    date->year    = ((dr >> 20) & 0xF) * 10 + ((dr >> 16) & 0xF);
    date->month   = ((dr >> 12) & 0x1) * 10 + ((dr >> 8) & 0xF);
    date->day     = ((dr >> 4)  & 0x3) * 10 + ((dr >> 0) & 0xF);
    date->weekday = ((dr >> 13) & 0x7);
    return 0;
}

/* ===================================================================
   闹钟 A/B
   =================================================================== */
static int rtc_set_alarm_internal(const rtc_alarm_config_t *alarm, bool is_alarm_a)
{
    if (!alarm) return -1;
    rtc_unlock();
    // 先关闭闹钟
    if (is_alarm_a) {
        xRTC->CR &= ~xRTC_CR_ALRAE;
        while (!(xRTC->ISR & xRTC_ISR_ALRAWF));
    } else {
        xRTC->CR &= ~xRTC_CR_ALRBE;
        while (!(xRTC->ISR & xRTC_ISR_ALRBWF));
    }
    // 配置闹钟寄存器
    volatile uint32_t *alarmr = is_alarm_a ? &xRTC->ALRMAR : &xRTC->ALRMBR;
    uint32_t reg = 0;
    reg |= (alarm->mask & 0xF) << 24;   // MSK[3:0]
    if (alarm->date_sel == RTC_ALARM_WEEKDAY)
        reg |= (1 << 30);               // WDSEL
    reg |= (alarm->date_or_weekday & 0x3F) << 24;  // DT/DY
    reg |= (alarm->time.am_pm & 1) << 22;
    reg |= ((alarm->time.hours   / 10) << 20) | ((alarm->time.hours   % 10) << 16);
    reg |= ((alarm->time.minutes / 10) << 12) | ((alarm->time.minutes % 10) << 8);
    reg |= ((alarm->time.seconds / 10) << 4)  |  (alarm->time.seconds % 10);
    *alarmr = reg;
    // 重新使能闹钟
    if (is_alarm_a)
        xRTC->CR |= xRTC_CR_ALRAE;
    else
        xRTC->CR |= xRTC_CR_ALRBE;
    rtc_lock();
    return 0;
}

int rtc_set_alarm_a(const rtc_alarm_config_t *alarm) {
    return rtc_set_alarm_internal(alarm, true);
}
int rtc_set_alarm_b(const rtc_alarm_config_t *alarm) {
    return rtc_set_alarm_internal(alarm, false);
}

int rtc_deactivate_alarm_a(void) {
    xRTC->CR &= ~xRTC_CR_ALRAE;
    return 0;
}
int rtc_deactivate_alarm_b(void) {
    xRTC->CR &= ~xRTC_CR_ALRBE;
    return 0;
}

/* ===================================================================
   唤醒定时器
   =================================================================== */
int rtc_set_wakeup(const rtc_wakeup_config_t *wakeup)
{
    if (!wakeup) return -1;
    rtc_unlock();
    // 关闭唤醒定时器
    xRTC->CR &= ~xRTC_CR_WUTE;
    while (!(xRTC->ISR & xRTC_ISR_WUTWF));
    // 配置唤醒时钟和重载值
    xRTC->CR = (xRTC->CR & ~(0x7 << 0)) | (wakeup->clk_src & 0x7);
    xRTC->WUTR = wakeup->reload;
    // 重新使能
    xRTC->CR |= xRTC_CR_WUTE;
    rtc_lock();
    return 0;
}

int rtc_deactivate_wakeup(void)
{
    xRTC->CR &= ~xRTC_CR_WUTE;
    return 0;
}

/* ===================================================================
   备份寄存器 (20个, index 0..19)
   =================================================================== */
int rtc_write_backup(uint8_t index, uint32_t data)
{
    if (index >= 20) return -1;
    rtc_unlock();
    xRTC->BKPR[index] = data;
    rtc_lock();
    return 0;
}

uint32_t rtc_read_backup(uint8_t index)
{
    if (index >= 20) return 0;
    return xRTC->BKPR[index];
}

// 备份 SRAM 操作
int rtc_write_backup_sram(uint8_t *data, uint16_t len)
{
    if (len > 4096) return -1;

    // 使能备份域写访问（已在 rtc_init 中完成，这里是安全检查）
    xPWR->CR |= xPWR_CR_DBP;

    // 备份 SRAM 直接通过指针访问
    uint8_t *p_sram = (uint8_t *)BACKUP_SRAM_BASE;
    for (uint16_t i = 0; i < len; i++) {
        p_sram[i] = data[i];
    }
    return 0;
}

int rtc_read_backup_sram(uint8_t *buffer, uint16_t len)
{
    if (len > 4096) return -1;

    uint8_t *p_sram = (uint8_t *)BACKUP_SRAM_BASE;
    for (uint16_t i = 0; i < len; i++) {
        buffer[i] = p_sram[i];
    }
    return 0;
}

/* ═══════════════ CR / ISR 原子操作 ═══════════════ */

void hal_rtc_set_hour_format(rtc_hour_format_t fmt)
{
    if (fmt == RTC_FORMAT_24H)
        xRTC->CR &= ~(1 << 6);
    else
        xRTC->CR |= (1 << 6);
}

void hal_rtc_write_cr(uint32_t cr)
{
    xRTC->CR = cr;
}

uint32_t hal_rtc_read_cr(void)
{
    return xRTC->CR;
}

uint32_t hal_rtc_read_isr(void)
{
    return xRTC->ISR;
}

void hal_rtc_clear_isr_flag(uint32_t flag)
{
    xRTC->ISR &= ~flag;
}

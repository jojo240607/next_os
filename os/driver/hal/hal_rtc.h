//
// Created by zhiwei.gong on 2026/5/14.
//

#ifndef STM32F4DISCOVERY_HAL_RTC_H
#define STM32F4DISCOVERY_HAL_RTC_H
#include "stdint.h"

/* ---------- RTC 寄存器基址 ---------- */
#define xRTC_BASE  0x40002800UL

/* ---------- RTC 寄存器结构 ---------- */
typedef struct {
    volatile uint32_t TR;           // 0x00 时间寄存器
    volatile uint32_t DR;           // 0x04 日期寄存器
    volatile uint32_t CR;           // 0x08 控制寄存器
    volatile uint32_t ISR;          // 0x0C 初始化与状态寄存器
    volatile uint32_t PRER;         // 0x10 预分频器
    volatile uint32_t WUTR;         // 0x14 唤醒定时器
    volatile uint32_t CALIBR;       // 0x18 校准寄存器
    volatile uint32_t ALRMAR;       // 0x1C 闹钟 A
    volatile uint32_t ALRMBR;       // 0x20 闹钟 B
    volatile uint32_t WPR;          // 0x24 写保护
    volatile uint32_t SSR;          // 0x28 亚秒
    volatile uint32_t SHIFTR;       // 0x2C 移位控制
    volatile uint32_t TSTR;         // 0x30 时间戳时间
    volatile uint32_t TSDR;         // 0x34 时间戳日期
    volatile uint32_t TSSSR;        // 0x38 时间戳亚秒
    volatile uint32_t CALR;         // 0x3C 校准
    volatile uint32_t TAFCR;        // 0x40 篡改与复用功能
    volatile uint32_t ALRMASSR;     // 0x44 闹钟 A 亚秒
    volatile uint32_t ALRMBSSR;     // 0x48 闹钟 B 亚秒
    uint32_t reserved0;
    volatile uint32_t BKPR[20];     // 0x50..0x9C 备份寄存器
} xRTC_TypeDef;

#define xRTC  ((xRTC_TypeDef *)xRTC_BASE)

/* ---------- 备份域控制寄存器 (RCC_BDCR) ---------- */
#define xRCC_BDCR_BASE  0x40023870UL
#define BACKUP_SRAM_BASE  0x40024000UL  // 备份域 SRAM 的基地址
typedef struct {
    volatile uint32_t BDCR;
} xRCC_BDCR_TypeDef;
#define xRCC_BDCR  ((xRCC_BDCR_TypeDef *)xRCC_BDCR_BASE)

/* ---------- 电源控制寄存器 (PWR_CR) ---------- */
#define xPWR_CR_BASE  0x40007000UL
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CSR;
} xPWR_TypeDef;
#define xPWR  ((xPWR_TypeDef *)xPWR_CR_BASE)
/* RCC_BDCR 位 */
#define xRCC_BDCR_LSEON     (1 << 0)
#define xRCC_BDCR_LSERDY    (1 << 1)
#define xRCC_BDCR_LSEBYP    (1 << 2)
#define xRCC_BDCR_RTCSEL_SHIFT  8
#define xRCC_BDCR_RTCSEL_MASK   (0x3 << xRCC_BDCR_RTCSEL_SHIFT)
#define xRCC_BDCR_RTCEN     (1 << 15)
#define xRCC_BDCR_BDRST     (1 << 16)

/* PWR_CR 位 */
#define xPWR_CR_DBP         (1 << 8)

/* RTC 控制寄存器位 */
#define xRTC_CR_WUTE        (1 << 10)
#define xRTC_CR_ALRAE       (1 << 8)
#define xRTC_CR_ALRBE       (1 << 9)
#define xRTC_CR_TSIE        (1 << 3)
#define xRTC_CR_WUTIE       (1 << 14)
#define xRTC_CR_ALRAIE      (1 << 12)
#define xRTC_CR_ALRBIE      (1 << 13)

/* RTC 初始化与状态寄存器位 */
#define xRTC_ISR_INITF      (1 << 6)
#define xRTC_ISR_INIT       (1 << 7)
#define xRTC_ISR_RSF        (1 << 5)
#define xRTC_ISR_WUTF       (1 << 10)
#define xRTC_ISR_ALRAF      (1 << 8)
#define xRTC_ISR_ALRBF      (1 << 9)
#define xRTC_ISR_TSF        (1 << 11)
#define xRTC_ISR_TAMP1F     (1 << 13)
#define xRTC_ISR_WUTWF      (1 << 2)
#define xRTC_ISR_ALRAWF     (1 << 0)
#define xRTC_ISR_ALRBWF     (1 << 1)

/* RTC_WPR 写保护键 */
#define xRTC_WPR_KEY1       0xCA
#define xRTC_WPR_KEY2       0x53


/* 时钟源 */
typedef enum : uint8_t {
    RTC_CLK_LSE = 0,
    RTC_CLK_LSI,
    RTC_CLK_HSE_DIV
} rtc_clk_src_t;

/* 小时格式 */
typedef enum : uint8_t {
    RTC_FORMAT_24H = 0,
    RTC_FORMAT_12H = 1
} rtc_hour_format_t;

/* 闹钟掩码 */
typedef enum : uint8_t {
    RTC_ALARM_MASK_NONE      = 0,   // 全部精确匹配
    RTC_ALARM_MASK_SECONDS   = 1,   // 忽略秒
    RTC_ALARM_MASK_MINUTES   = 2,   // 忽略分钟
    RTC_ALARM_MASK_HOURS     = 3,   // 忽略小时
    RTC_ALARM_MASK_DAY       = 4,   // 忽略日期
    RTC_ALARM_MASK_WEEKDAY   = 4    // 忽略星期
} rtc_alarm_mask_t;

/* 日期星期选择 */
typedef enum : uint8_t {
    RTC_ALARM_DATE = 0,
    RTC_ALARM_WEEKDAY = 1
} rtc_alarm_date_sel_t;

/* 唤醒时钟分频 */
typedef enum : uint8_t {
    RTC_WAKEUP_CLK_RTCCLK_DIV16    = 0,  // RTCCLK / 16
    RTC_WAKEUP_CLK_RTCCLK_DIV8     = 1,  // RTCCLK / 8
    RTC_WAKEUP_CLK_RTCCLK_DIV4     = 2,  // RTCCLK / 4
    RTC_WAKEUP_CLK_RTCCLK_DIV2     = 3,  // RTCCLK / 2
    RTC_WAKEUP_CLK_CK_SPRE         = 4,  // ck_spre (1Hz)
    RTC_WAKEUP_CLK_CK_SPRE_2       = 5,  // ck_spre，但 WUCKSEL=0x10
    RTC_WAKEUP_CLK_CK_SPRE_3       = 6,  // ck_spre，但 WUCKSEL=0x11
} rtc_wakeup_clk_t;

/* 中断使能选项 */
typedef enum :uint8_t {
    RTC_IT_ALARM_A   = (1 << 0),
    RTC_IT_ALARM_B   = (1 << 1),
    RTC_IT_WAKEUP    = (1 << 2),
    RTC_IT_TIMESTAMP = (1 << 3),
    RTC_IT_TAMPER    = (1 << 4),
} rtc_it_t;

/* 中断事件 */
typedef enum {
    RTC_EVT_ALARM_A = 0,
    RTC_EVT_ALARM_B,
    RTC_EVT_WAKEUP,
    RTC_EVT_TIMESTAMP,
    RTC_EVT_TAMPER
} rtc_event_t;

/* 时间结构体 (BCD) */
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t am_pm;     // 0=AM, 1=PM (12小时制)
} rtc_time_t;

/* 日期结构体 (BCD) */
typedef struct {
    uint8_t weekday;   // 1=Monday, 7=Sunday
    uint8_t day;
    uint8_t month;     // 1=January
    uint8_t year;      // 0..99 (20xx)
} rtc_date_t;

/* 闹钟配置描述符 */
typedef struct {
    rtc_time_t           time;
    rtc_alarm_mask_t     mask;
    rtc_alarm_date_sel_t date_sel;
    uint8_t              date_or_weekday;
} rtc_alarm_config_t;

/* 唤醒配置描述符 */
typedef struct {
    rtc_wakeup_clk_t     clk_src;
    uint16_t             reload;       // 0..0xFFFF
} rtc_wakeup_config_t;

int rtc_enable_backup_domain(void);
int rtc_select_clock_source(rtc_clk_src_t src);
void rtc_unlock(void);
int rtc_enter_init_mode(void);
void rtc_exit_init_mode(void);
void rtc_wait_sync(void);
void rtc_lock(void);

/* 备份 SRAM 内存操作 */
int  rtc_write_backup_sram(uint8_t *data, uint16_t len);
int  rtc_read_backup_sram(uint8_t *buffer, uint16_t len);

#endif //STM32F4DISCOVERY_HAL_RTC_H

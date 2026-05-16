#ifndef RCC_H
#define RCC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* RCC AHB1时钟使能位 */

typedef enum :uint8_t {
    xRCC_AHB1ENR_GPIOAEN       = (0),
    xRCC_AHB1ENR_GPIOBEN       = (1),
    xRCC_AHB1ENR_GPIOCEN       = (2),
    xRCC_AHB1ENR_GPIODEN       = (3),
    xRCC_AHB1ENR_GPIOEEN       = (4),
    xRCC_AHB1ENR_GPIOFEN       = (5),
    xRCC_AHB1ENR_GPIOGEN       = (6),
    xRCC_AHB1ENR_GPIOHEN       = (7),
    xRCC_AHB1ENR_GPIOIEN       = (8),
    xRCC_AHB1ENR_CRCEN         = (12),
    xRCC_AHB1ENR_BKPSRAMEN     = (18),
    xRCC_AHB1ENR_CCMDATARAMEN  = (19),
    xRCC_AHB1ENR_DMA1EN        = (20),
    xRCC_AHB1ENR_DMA2EN        = (21),
    xRCC_AHB1ENR_ETHMACEN      = (25),
    xRCC_AHB1ENR_ETHMACTXEN    = (26),
    xRCC_AHB1ENR_ETHMACRXEN    = (27),
    xRCC_AHB1ENR_ETHMACPTPEN   = (28),
    xRCC_AHB1ENR_OTGHSEN       = (29),
    xRCC_AHB1ENR_OTGHSULPIEN   = (30),

/* RCC AHB2时钟使能位 */
    xRCC_AHB2ENR_DCMIEN        = (0),
    xRCC_AHB2ENR_OTGFSEN       = (7),

/* RCC AHB3时钟使能位 */
    xRCC_AHB3ENR_FSMCEN        = (0),

/* RCC APB1 时钟使能位 */
    xRCC_APB1ENR_TIM2EN     = (0),
    xRCC_APB1ENR_TIM3EN     = (1),
    xRCC_APB1ENR_TIM4EN     = (2),
    xRCC_APB1ENR_TIM5EN     = (3),
    xRCC_APB1ENR_TIM6EN     = (4),
    xRCC_APB1ENR_TIM7EN     = (5),
    xRCC_APB1ENR_TIM12EN    = (6),
    xRCC_APB1ENR_TIM13EN    = (7),
    xRCC_APB1ENR_TIM14EN    = (8),
    xRCC_APB1ENR_SDIOEN     = (11),
    xRCC_APB1ENR_SPI2EN     = (14),
    xRCC_APB1ENR_SPI3EN     = (15),
    xRCC_APB1ENR_I2S2EN     = (14),
    xRCC_APB1ENR_I2S3EN     = (15),
    xRCC_APB1ENR_USART2EN   = (17),
    xRCC_APB1ENR_USART3EN   = (18),
    xRCC_APB1ENR_UART4EN    = (19),
    xRCC_APB1ENR_UART5EN    = (20),
    xRCC_APB1ENR_I2C1EN     = (21),
    xRCC_APB1ENR_I2C2EN     = (22),
    xRCC_APB1ENR_I2C3EN     = (23),
    xRCC_APB1ENR_CAN1EN     = (25),   /* CAN1 时钟 */
    xRCC_APB1ENR_CAN2EN     = (26),   /* CAN2 时钟 */
    xRCC_APB1ENR_PWREN      = (28),   /* 电源接口时钟 */
    xRCC_APB1ENR_DACEN      = (29),   /* DAC 时钟 */

/* RCC APB2时钟使能位 */
    xRCC_APB2ENR_TIM1EN     = (0),
    xRCC_APB2ENR_TIM8EN     = (1),
    xRCC_APB2ENR_USART1EN   = (4),
    xRCC_APB2ENR_USART6EN   = (5),
    xRCC_APB2ENR_ADC1EN     = (8),
    xRCC_APB2ENR_ADC2EN     = (9),
    xRCC_APB2ENR_ADC3EN     = (10),
    xRCC_APB2ENR_SPI1EN     = (12),
    xRCC_APB2ENR_SYSCFGEN   = (14),   /* SYSCFG 时钟 (EXTI 模块需要) */
    xRCC_APB2ENR_TIM9EN     = (16),
    xRCC_APB2ENR_TIM10EN    = (17),
    xRCC_APB2ENR_TIM11EN    = (18),

} rcc_enter_t;
/* ---------- 时钟源选择 ---------- */
typedef enum : uint8_t {
    RCC_CLK_HSI = 0,
    RCC_CLK_HSE,
    RCC_CLK_PLL
} rcc_sysclk_src_t;

/* ---------- PLL 时钟源 ---------- */
typedef enum : uint8_t {
    RCC_PLLSRC_HSI = 0,
    RCC_PLLSRC_HSE = 1
} rcc_pllsrc_t;

/* ---------- 系统时钟分频 ---------- */
typedef enum : uint8_t {
    RCC_AHB_DIV1   = 0x00,
    RCC_AHB_DIV2   = 0x08,
    RCC_AHB_DIV4   = 0x09,
    RCC_AHB_DIV8   = 0x0A,
    RCC_AHB_DIV16  = 0x0B,
    RCC_AHB_DIV64  = 0x0C,
    RCC_AHB_DIV128 = 0x0D,
    RCC_AHB_DIV256 = 0x0E,
    RCC_AHB_DIV512 = 0x0F
} rcc_ahb_div_t;

typedef enum : uint8_t {
    RCC_APB_DIV1  = 0x00,
    RCC_APB_DIV2  = 0x04,
    RCC_APB_DIV4  = 0x05,
    RCC_APB_DIV8  = 0x06,
    RCC_APB_DIV16 = 0x07
} rcc_apb_div_t;

/* ---------- 外设总线标签 ---------- */
typedef enum : uint8_t {
    RCC_BUS_AHB1,
    RCC_BUS_AHB2,
    RCC_BUS_AHB3,
    RCC_BUS_APB1,
    RCC_BUS_APB2
} rcc_bus_t;

/* ---------- 系统时钟配置描述符 ---------- */
typedef struct {
    rcc_sysclk_src_t  sysclk_src;      // 系统时钟来源
    rcc_pllsrc_t      pll_src;         // PLL输入源 (HSE / HSI)
    bool              hse_bypass;      // HSE旁路? 若使用外部有源晶振则为true
    uint32_t          hse_freq;        // HSE频率 (Hz)
    uint32_t          hsi_freq;        // HSI频率 (Hz, 通常16MHz)

    /* PLL 参数 (PLL = VCO / (M * N * P * Q) ) */
    uint32_t          pll_m;           // PLLM 分频 (2~63)
    uint32_t          pll_n;           // PLLN 倍频 (64~432)
    uint32_t          pll_p;           // PLLP 分频 (2/4/6/8)
    uint32_t          pll_q;           // PLLQ 分频 (for USB/音频等, 2~15)

    /* 总线分频 */
    rcc_ahb_div_t     ahb_div;
    rcc_apb_div_t     apb1_div;
    rcc_apb_div_t     apb2_div;

    /* 期望的系统时钟频率 (Hz)，用于验证结果 */
    uint32_t          target_sysclk;
} rcc_sysclk_config_t;

/* ---------- 外设时钟使能/禁用接口 (按总线+位号) ---------- */
void rcc_periph_clock_enable(rcc_bus_t bus, rcc_enter_t periph_bit);
void rcc_periph_clock_disable(rcc_bus_t bus, rcc_enter_t periph_bit);

/* ---------- 外设复位控制 ---------- */
void rcc_periph_reset_assert(rcc_bus_t bus, rcc_enter_t periph_bit);
void rcc_periph_reset_deassert(rcc_bus_t bus, rcc_enter_t periph_bit);

/* ---------- 系统复位 ---------- */
void rcc_system_reset(void);

/* ---------- 全局初始化 ---------- */
bool rcc_sysclk_init(const rcc_sysclk_config_t *cfg);

/* ---------- 获取当前时钟频率 ---------- */
uint32_t rcc_get_system_clock(void);
uint32_t rcc_get_ahb_clock(void);
uint32_t rcc_get_apb1_clock(void);
uint32_t rcc_get_apb2_clock(void);

#endif /* RCC_H */
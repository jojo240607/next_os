//
// Created by zhiwei.gong on 2026/5/19.
//

#include "hal_dac.h"
#include "../common/rcc.h"
#include "../common/nvic.h"
#include <stddef.h>

/* ───────── DAC 寄存器定义 ───────── */
typedef struct {
    volatile uint32_t CR;       // 控制寄存器
    volatile uint32_t SWTRIGR;  // 软件触发寄存器
    volatile uint32_t DHR12R1;  // 通道1 12位右对齐数据保持寄存器
    volatile uint32_t DHR12L1;  // 通道1 12位左对齐
    volatile uint32_t DHR8R1;   // 通道1 8位右对齐
    volatile uint32_t DHR12R2;  // 通道2 12位右对齐
    volatile uint32_t DHR12L2;  // 通道2 12位左对齐
    volatile uint32_t DHR8R2;   // 通道2 8位右对齐
    volatile uint32_t DHR12RD;  // 双通道12位右对齐
    volatile uint32_t DHR12LD;  // 双通道12位左对齐
    volatile uint32_t DHR8RD;   // 双通道8位右对齐
    volatile uint32_t DOR1;     // 通道1输出数据
    volatile uint32_t DOR2;     // 通道2输出数据
    volatile uint32_t SR;       // 状态寄存器
} xDAC_TypeDef;

#define xDAC_BASE  0x40007400UL
#define xDAC       ((xDAC_TypeDef *)xDAC_BASE)

/* 控制寄存器位 */
#define xDAC_CR_EN1      (1 << 0)   // 通道1使能
#define xDAC_CR_BOFF1    (1 << 1)   // 通道1输出缓冲关闭
#define xDAC_CR_TEN1     (1 << 2)   // 通道1触发使能
#define xDAC_CR_TSEL1_Pos 3
#define xDAC_CR_WAVE1_Pos  6
#define xDAC_CR_MAMP1_Pos  8         // 掩码/幅值 (波形时使用)
#define xDAC_CR_DMAEN1   (1 << 12)  // 通道1 DMA使能
#define xDAC_CR_DMAUDRIE1 (1 << 13) // 通道1 DMA下溢中断使能

#define xDAC_CR_EN2      (1 << 16)
#define xDAC_CR_BOFF2    (1 << 17)
#define xDAC_CR_TEN2     (1 << 18)
#define xDAC_CR_TSEL2_Pos 19
#define xDAC_CR_WAVE2_Pos 22
#define xDAC_CR_DMAEN2   (1 << 28)
#define xDAC_CR_DMAUDRIE2 (1 << 29)

/* 状态寄存器 */
#define xDAC_SR_DMAUDR1  (1 << 13)
#define xDAC_SR_DMAUDR2  (1 << 29)

/* ───────── 内部状态 ───────── */
static bool dac_inited = false;
static dac_callback_t dac_user_cb = NULL;

/* ===================================================================
   初始化
   =================================================================== */
int hal_dac_init(const dac_config_t *cfg)
{
    if (!cfg || dac_inited) return -1;

    /* 1. 使能 DAC 时钟 (APB1 位29) */
    rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_DACEN);

    /* 2. 配置引脚为模拟模式 */
    for (int i = 0; i < cfg->num_channels; i++) {
        const dac_channel_config_t *ch = &cfg->channels[i];
        pin_config_t pin_cfg = {
                .port   = ch->dac_pin.port,
                .pin    = ch->dac_pin.pin,
                .mode   = PIN_MODE_ANALOG,
                .otype  = PIN_OTYPE_PP,
                .ospeed = PIN_OSPEED_LOW,
                .pupd   = PIN_PUPD_NONE,
                .af     = 0
        };
        if (pinmux_request(&pin_cfg) != 0) return -1;
    }

    /* 3. 配置各通道 */
    for (int i = 0; i < cfg->num_channels; i++) {
        const dac_channel_config_t *ch = &cfg->channels[i];
        uint32_t cr_val = 0;

        if (ch->channel == DAC_CHANNEL_1) {
            cr_val = xDAC_CR_EN1;   // 使能通道1
            if (ch->buffer == DAC_BUFFER_DISABLE) cr_val |= xDAC_CR_BOFF1;
            if (ch->trigger != DAC_TRIGGER_NONE) {
                cr_val |= xDAC_CR_TEN1;
                cr_val |= (ch->trigger & 0x0F) << xDAC_CR_TSEL1_Pos;
            }
            if (ch->wave != DAC_WAVE_NONE) {
                cr_val |= (ch->wave & 0x03) << xDAC_CR_WAVE1_Pos;
                // 幅值设置（噪声/三角波需要，这里默认最大值）
                cr_val |= (0x0F) << xDAC_CR_MAMP1_Pos; // 最大幅值
            }
            if (ch->enable_dma) {
                cr_val |= xDAC_CR_DMAEN1;
                if (ch->enable_irq) cr_val |= xDAC_CR_DMAUDRIE1;
            }
            /* 用读-改-写更新 CR 的高半部分（通道1） */
            xDAC->CR = (xDAC->CR & 0xFFFF0000) | cr_val;
        } else {
            cr_val = xDAC_CR_EN2;
            if (ch->buffer == DAC_BUFFER_DISABLE) cr_val |= xDAC_CR_BOFF2;
            if (ch->trigger != DAC_TRIGGER_NONE) {
                cr_val |= xDAC_CR_TEN2;
                cr_val |= (ch->trigger & 0x0F) << xDAC_CR_TSEL2_Pos;
            }
            if (ch->wave != DAC_WAVE_NONE) {
                cr_val |= (ch->wave & 0x03) << xDAC_CR_WAVE2_Pos;
                cr_val |= (0x0F) << (xDAC_CR_MAMP1_Pos + 16);
            }
            if (ch->enable_dma) {
                cr_val |= xDAC_CR_DMAEN2;
                if (ch->enable_irq) cr_val |= xDAC_CR_DMAUDRIE2;
            }
            xDAC->CR = (xDAC->CR & 0x0000FFFF) | cr_val;
        }
    }

    /* 4. NVIC 使能（如果有中断需求） */
    // DAC 中断与 DMA 下溢相关，可选择性使能 NVIC
    // nvic_enable_irq(DAC_IRQn);

    dac_inited = true;
    return 0;
}

void hal_dac_deinit(dac_id_t id)
{
    (void)id;
    if (!dac_inited) return;
    xDAC->CR = 0;
    dac_inited = false;
}

/* ===================================================================
   软件设置输出电压 (12位右对齐)
   =================================================================== */
int hal_dac_set_value(dac_id_t id, dac_channel_t channel, uint16_t value)
{
    (void)id;
    if (!dac_inited) return -1;
    if (channel == DAC_CHANNEL_1)
        xDAC->DHR12R1 = value & 0x0FFF;
    else
        xDAC->DHR12R2 = value & 0x0FFF;

    /* 若触发使能，需等待触发；若无触发，立即更新输出 */
    return 0;
}

/* ===================================================================
   DMA 模式启动 / 停止
   =================================================================== */
int hal_dac_start_dma(dac_id_t id, dac_channel_t channel, const uint16_t *data, uint16_t len)
{
    (void)id;
    if (!dac_inited || !data || len == 0) return -1;

    // 这里假设用户已经配置了与 DAC 关联的 DMA 流
    // 实际应用中需结合 DMA 框架，此处只给出接口
    // dma_start_transfer(ctrl, stream, mem_addr, &DAC->DHR12R1, len);
    return 0;
}

int hal_dac_stop_dma(dac_id_t id, dac_channel_t channel)
{
    (void)id;
    if (!dac_inited) return -1;
    // 关闭对应通道的 DMA 使能位
    if (channel == DAC_CHANNEL_1) {
        xDAC->CR &= ~xDAC_CR_DMAEN1;
    } else {
        xDAC->CR &= ~xDAC_CR_DMAEN2;
    }
    return 0;
}

/* ===================================================================
   中断回调
   =================================================================== */
void hal_dac_register_callback(dac_id_t id, dac_callback_t callback)
{
    (void)id;
    dac_user_cb = callback;
}

void hal_dac_irq_handler(void)
{
    uint32_t sr = xDAC->SR;
    if (sr & xDAC_SR_DMAUDR1) {
        if (dac_user_cb) dac_user_cb(DAC_1, DAC_EVT_DMA_UNDERRUN);
    }
    if (sr & xDAC_SR_DMAUDR2) {
        if (dac_user_cb) dac_user_cb(DAC_1, DAC_EVT_DMA_UNDERRUN);
    }
}

/* 芯片中断入口 (若使用) */
void DAC_IRQHandler(void) { hal_dac_irq_handler(); }
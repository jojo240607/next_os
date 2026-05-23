//
// Created by zhiwei.gong on 2026/5/19.
//

#ifndef STM32F4DISCOVERY_HAL_DAC_H
#define STM32F4DISCOVERY_HAL_DAC_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/pinmux.h"
#include "../common/dma.h"
#include "../common/gpio.h"

/* DAC 实例 */
typedef enum {
    DAC_1 = 0,
    DAC_2,
    DAC_MAX
} dac_id_t;

/* DAC 通道 */
//DAC通道1（DAC_OUT1）：对应PA4引脚
//DAC通道2（DAC_OUT2）：对应PA5引脚
typedef enum {
    DAC_CHANNEL_1 = 0,
    DAC_CHANNEL_2 = 1
} dac_channel_t;

/* 触发源 */
typedef enum {
    DAC_TRIGGER_NONE       = 0x00,   // 软件触发
    DAC_TRIGGER_TIM2       = 0x04,   // TIM2_TRGO
    DAC_TRIGGER_TIM4       = 0x05,   // TIM4_TRGO
    DAC_TRIGGER_TIM5       = 0x06,   // TIM5_TRGO
    DAC_TRIGGER_TIM6       = 0x07,   // TIM6_TRGO
    DAC_TRIGGER_TIM7       = 0x08,   // TIM7_TRGO
    DAC_TRIGGER_TIM8       = 0x09,   // TIM8_TRGO
    DAC_TRIGGER_EXTI_9     = 0x0A,   // EXTI Line9
    DAC_TRIGGER_SOFTWARE   = 0x0C    // 软件触发
} dac_trigger_t;

/* 波形生成 */
typedef enum {
    DAC_WAVE_NONE      = 0x00,   // 无波形
    DAC_WAVE_NOISE     = 0x01,   // 噪声波形
    DAC_WAVE_TRIANGLE  = 0x02    // 三角波
} dac_wave_t;

/* 输出缓冲 */
typedef enum {
    DAC_BUFFER_ENABLE  = 0,
    DAC_BUFFER_DISABLE = 1
} dac_buffer_t;

/* DAC 通道配置 */
typedef struct {
    dac_channel_t   channel;        // 通道 1 或 2
    dac_trigger_t   trigger;        // 触发源
    dac_wave_t      wave;           // 波形生成
    dac_buffer_t    buffer;         // 输出缓冲
    bool            enable_dma;     // 使能 DMA
    bool            enable_irq;     // 使能中断 (DMA下溢)
    gpio_t          dac_pin;        // 对应引脚端口 引脚号
} dac_channel_config_t;

/* DAC 总配置 */
typedef struct {
    dac_id_t                id;              // DAC1 或 DAC2 (实际上 STM32F407 只有一个 DAC 模块包含两个通道)
    uint8_t                 num_channels;    // 使用的通道数量
    dac_channel_config_t    channels[2];     // 最多两个通道
} dac_config_t;

/* 中断事件 */
typedef enum {
    DAC_EVT_DMA_UNDERRUN = 0
} dac_event_t;

typedef void (*dac_callback_t)(dac_id_t id, dac_event_t event);

/* ========== API ========== */
int  hal_dac_init(const dac_config_t *cfg);
void hal_dac_deinit(dac_id_t id);

/* 设置输出电压 (软件触发模式) */
int  hal_dac_set_value(dac_id_t id, dac_channel_t channel, uint16_t value);

/* 启动 / 停止 DMA 波形输出 */
int  hal_dac_start_dma(dac_id_t id, dac_channel_t channel, const uint16_t *data, uint16_t len);
int  hal_dac_stop_dma(dac_id_t id, dac_channel_t channel);

/* 中断回调注册 */
void hal_dac_register_callback(dac_id_t id, dac_callback_t callback);

/* 中断处理 (由芯片 ISR 调用) */
void hal_dac_irq_handler(void);
#endif //STM32F4DISCOVERY_HAL_DAC_H

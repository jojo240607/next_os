#ifndef ADC_H
#define ADC_H
#include <stdint.h>
#include <stdbool.h>
#include "../common/device.h"
#include "../common/pinmux.h"
#include "../common/dma.h"
#include "../hal/hal_adc.h"
#include "../../scheduler/semaphore.h"

#define GET_ADC_VTABLE(obj) GET_DEVICE_VTABLE(obj) //(*(AdcVTable **)obj)
#define GET_ADC(obj) ((Adc *)obj)
#define ADC_SUCCESS 1
#define ADC_ERROR 0

/* ADC 事件 */
typedef enum : uint8_t {
    ADC_XFER_START = 1,
    ADC_XFER_DONE,
} adc_dev_event_t;
// 派生类声明
typedef struct _Adc Adc;
typedef struct _AdcFun AdcFun;
// 类成员函数结构
struct _AdcFun {
    void (*destroy)(Adc* self);
	int (*start_dma)(Adc* self, uint16_t *buffer, uint16_t count);

};


typedef enum : uint8_t {
    ADC_RES_12BIT = 0,
    ADC_RES_10BIT,
    ADC_RES_8BIT,
    ADC_RES_6BIT
} adc_resolution_t;

typedef enum : uint8_t {
    ADC_ALIGN_RIGHT = 0,
    ADC_ALIGN_LEFT  = 1
} adc_align_t;

/* 采样时间 */
typedef enum :uint8_t {
    ADC_SMP_3CYCLES = 0x00,
    ADC_SMP_15CYCLES,
    ADC_SMP_28CYCLES,
    ADC_SMP_56CYCLES,
    ADC_SMP_84CYCLES,
    ADC_SMP_112CYCLES,
    ADC_SMP_144CYCLES,
    ADC_SMP_480CYCLES,
} adc_sample_t;


typedef struct {
    adc_id_t id;
    adc_resolution_t  resolution;
    adc_align_t       align;
    bool              continuous;      /* 连续转换模式 (若不使用 DMA 请慎用) */
    /* DMA 可选配置 */
    const dma_stream_config_t *dma_cfg;   /* 为 NULL 则不使用 DMA */
    uint8_t           num_channels;    /* 规则通道数量 */
    const adc_channel_cfg_t * const channels[];    /* 最多 16 个规则通道 */
} adc_config_t;

struct _Adc {
    Device base;  // 基类作为第一个成员
    const AdcFun* fun;
    Semaphore     *adc_sem;            /* ADC 完成信号量 (listener 使用) */
};

// 构造函数声明
Adc* adc_create(const device_info_t *info);
void adc_init(Adc* self, const device_info_t *info);

// 析构函数声明
void adc_deinit(Adc* self);

#endif // ADC_H
#include "dma.h"
#include "rcc.h"
#include "../../log/log.h"
#include "../../common/linear_pool.h"


/* 获取 stream 寄存器指针 */
static inline xDMA_Stream_TypeDef* DMA_Stream(const dma_stream_config_t *cfg)
{
    uint32_t base = (DMA_REQ_GET_CTRL(cfg->dma_request) == DMA_1) ?
            DMA1_STREAM_BASE(DMA_REQ_GET_STREAM(cfg->dma_request)) : DMA2_STREAM_BASE(DMA_REQ_GET_STREAM(cfg->dma_request));
    return (xDMA_Stream_TypeDef*)base;
}

/* 全局流占用表 */
typedef struct {
    dma_stream_config_t  config;
    bool                 allocated;
} dma_stream_state_t;

static dma_stream_state_t *dma_states[xDMA_CONTROLLER_MAX][8];

void dma_init(void)
{
    LOG_DEBUG("dma", "dma init");
    for (int c = 0; c < xDMA_CONTROLLER_MAX; c++) {
        for (int s = 0; s < 8; s++) {
            dma_states[c][s] = NULL;
        }
    }
}

int dma_stream_request(const dma_stream_config_t *cfg)
{
    if (DMA_REQ_GET_CTRL(cfg->dma_request) >= xDMA_CONTROLLER_MAX || DMA_REQ_GET_STREAM(cfg->dma_request) > 7) {
        return DMA_ERROR;
    }

    dma_stream_state_t *st = dma_states[DMA_REQ_GET_CTRL(cfg->dma_request)][DMA_REQ_GET_STREAM(cfg->dma_request)];
    if (!st) {
        st = os_malloc(sizeof(dma_stream_state_t));
        memset(st, 0, sizeof(dma_stream_state_t));
        dma_states[DMA_REQ_GET_CTRL(cfg->dma_request)][DMA_REQ_GET_STREAM(cfg->dma_request)] = st;
    }

    if (st->allocated) {
        /* 简单冲突检查：只要占用就不让用（也可按需求宽松处理） */
        LOG_ERROR("dma", "error dma %d stream %d have allocated", DMA_REQ_GET_CTRL(cfg->dma_request), DMA_REQ_GET_STREAM(cfg->dma_request));
        return DMA_ERROR;
    }
    LOG_DEBUG("dma", "dma init ctrl %d, stream %d", DMA_REQ_GET_CTRL(cfg->dma_request), DMA_REQ_GET_STREAM(cfg->dma_request));
    /* 使能时钟 */
    dma_clock_enable(DMA_REQ_GET_CTRL(cfg->dma_request));

    /* 配置流寄存器 */
    xDMA_Stream_TypeDef *dma = DMA_Stream(cfg);
    /* 先关闭流 */
    dma->SxCR = 0;
    /* 设置通道 */
    //  #define DMA_CHANNEL_0                 0x00000000U    /*!< DMA Channel 0 */
    //  #define DMA_CHANNEL_1                 0x02000000U    /*!< DMA Channel 1 */
    //  #define DMA_CHANNEL_2                 0x04000000U    /*!< DMA Channel 2 */
    //  #define DMA_CHANNEL_3                 0x06000000U    /*!< DMA Channel 3 */
    //  #define DMA_CHANNEL_4                 0x08000000U    /*!< DMA Channel 4 */
    //  #define DMA_CHANNEL_5                 0x0A000000U    /*!< DMA Channel 5 */
    //  #define DMA_CHANNEL_6                 0x0C000000U    /*!< DMA Channel 6 */
    //  #define DMA_CHANNEL_7                 0x0E000000U    /*!< DMA Channel 7 */ 1110 0000
    volatile uint32_t cr = (DMA_REQ_GET_CHANNEL(cfg->dma_request) & 0x7) << 25;
    /* 方向：注意 bit6=DIR, bit7=DIR 搭配？参考手册：位6/7用于方向控制（双缓冲模式下） */
    /*  #define DMA_SxCR_DIR_0           (0x1UL << DMA_SxCR_DIR_Pos)                    !< 0x00000040
        #define DMA_SxCR_DIR_1           (0x2UL << DMA_SxCR_DIR_Pos)                    !< 0x00000080
     *  #define DMA_PERIPH_TO_MEMORY          0x00000000U                 //!< Peripheral to memory direction
        #define DMA_MEMORY_TO_PERIPH          ((uint32_t)DMA_SxCR_DIR_0) 1 //!< Memory to peripheral direction
        #define DMA_MEMORY_TO_MEMORY          ((uint32_t)DMA_SxCR_DIR_1) 2 //!< Memory to memory direction
     * */
    if (cfg->direction == DMA_DIR_P2M) {
        cr |= (0x00 << 6);   /* 外设到存储器 */
    } else if (cfg->direction == DMA_DIR_M2P) {
        cr |= (0x01 << 6);   /* 存储器到外设 */
    } else {
        cr |= (0x02 << 6);   /* 存储器到存储器 */
    }
    /*
     * PFCTRL = 0：DMA 控制流（这是你需要的）。DMA 会在传输完设定的所有数据后自动停止。
     * PFCTRL = 1：外设控制流。
     *
     * */
    cr |= (0x0 << 5);
    /* 优先级 */
    cr |= (cfg->priority & 0x3) << 16;
    /* 数据宽度 */
    cr |= (cfg->mem_data_size & 0x3) << 13;
    cr |= (cfg->per_data_size & 0x3) << 11;
    /* 地址递增 */
    if (cfg->mem_inc) {
        cr |= (1 << 10);
    }
    if (cfg->per_inc) {
        cr |= (1 << 9);
    }
    /* 循环模式 */
    if (cfg->mode == DMA_MODE_CIRCULAR) {
        cr |= (1 << 8);//#define DMA_SxCR_CIRC_Pos        (8U)
    }
    /* 传输完成中断使能（暂时不开，可扩展） */
    dma->SxCR = cr;
    if (cfg->it_enable & xDMA_IT_TC) {  /* 传输完成中断 */
        dma->SxCR |= (1 << 4);   // TCIE
    }
    if (cfg->it_enable & xDMA_IT_HT) {  /* 半传输中断 */
        dma->SxCR |= (1 << 3);   // HTIE
    }
    if (cfg->it_enable & xDMA_IT_TE) {  /* 传输错误中断 */
        dma->SxCR |= (1 << 2);   // TEIE
    }

    /* FIFO 配置 */
    uint32_t fcr = 0;
    if (cfg->fifo_mode == DMA_FIFO_ENABLE) {
        fcr |= (1 << 2);        /* DMDIS=0, 使能直接模式？实际手册：FCR bit2 为 DMDIS，置0表示直接模式禁止，即使用FIFO */
        fcr |= (0x3 << 0);      /* FTH 满阈值，例如 1/2 */
    }
    dma->SxFCR = fcr;

    st->allocated = true;
    st->config = *cfg;
    return DMA_SUCCESS;
}

int dma_stream_release(const dma_stream_config_t *cfg)
{
    if (DMA_REQ_GET_CTRL(cfg->dma_request) >= xDMA_CONTROLLER_MAX || DMA_REQ_GET_STREAM(cfg->dma_request) > 7) {
        return DMA_ERROR;
    }
    dma_stream_state_t *st = dma_states[DMA_REQ_GET_CTRL(cfg->dma_request)][DMA_REQ_GET_STREAM(cfg->dma_request)];
    if (!st) {
        return DMA_SUCCESS;
    }
    st->allocated = false;
    /* 关闭流 */
    xDMA_Stream_TypeDef *dma = DMA_Stream(cfg);
    dma->SxCR = 0;
    return DMA_SUCCESS;
}

int dma_start_transfer(const dma_stream_config_t *cfg,
                       uint32_t src_addr, uint32_t dst_addr, uint16_t count)
{
    dma_stream_state_t *st = dma_states[DMA_REQ_GET_CTRL(cfg->dma_request)][DMA_REQ_GET_STREAM(cfg->dma_request)];
    if (!st) {
        return DMA_ERROR;
    }
    if (!st->allocated) {
        return DMA_ERROR;
    }
    xDMA_Stream_TypeDef *dma = DMA_Stream(cfg);

    /* 停止当前传输 */
    dma->SxCR &= ~(1 << 0);
    /* 清除标志 */
    dma_clear_flag(cfg);
    /* 设置地址和数量 */
    //SxPAR (外设地址寄存器)
    //SxM0AR (存储器地址 0 寄存器)
    //外设到存储器 (P2M)：外设是源 (SxPAR 是源地址)，存储器是目标 (SxM0AR 是目标地址)。
    //存储器到外设 (M2P)：存储器是源 (SxM0AR 是源地址)，外设是目标 (SxPAR 是目标地址)。
    if (cfg->direction == DMA_DIR_P2M) {
        dma->SxPAR  = src_addr;   /* 根据方向，这里是外设地址 或 源 */
        dma->SxM0AR = dst_addr;   /* 存储器地址 或 目标 */
    } else if (cfg->direction == DMA_DIR_M2P) {
        dma->SxPAR  = dst_addr;   /* 根据方向，这里是外设地址 或 源 */
        dma->SxM0AR = src_addr;   /* 存储器地址 或 目标 */
    } else {
        dma->SxPAR  = (uint32_t)dst_addr;   // 目标内存
        dma->SxM0AR = (uint32_t)src_addr;   // 源内存
    }

    dma->SxNDTR = count;
    /* 使能流 */
    dma->SxCR |= 1;
    // 轮询 TC 标志（例如 DMA2_Stream7）
    //uint32_t base = DMA2_STREAM_BASE(DMA_REQ_GET_STREAM(cfg->dma_request));
    //xDMA_Base_TypeDef *dma2 = (xDMA_Base_TypeDef *)base;
    //while (!(dma2->HISR & (1 << 26))); // TCIF7 位，请根据实际流调整
    return DMA_SUCCESS;
}

int dma_stop_transfer(const dma_stream_config_t *cfg)
{
    dma_stream_state_t *st = dma_states[DMA_REQ_GET_CTRL(cfg->dma_request)][DMA_REQ_GET_STREAM(cfg->dma_request)];
    if (!st) {
        return DMA_ERROR;
    }
    if (!st->allocated) {
        return DMA_ERROR;
    }
    xDMA_Stream_TypeDef *dma = DMA_Stream(cfg);
    dma->SxCR &= ~(1 << 0);
    dma_clear_flag(cfg);
    return DMA_SUCCESS;
}

/* 简单忙检测：传输完成标志 */
bool dma_is_busy(const dma_stream_config_t *cfg)
{
    xDMA_Stream_TypeDef *dma = DMA_Stream(cfg);
    return (dma->SxNDTR != 0) && (dma->SxCR & 1);
}
/*
 * #define DMA_HIFCR_CTCIF7_Pos     (27U)
 * #define DMA_HIFCR_CFEIF7_Pos     (22U)
 *
 * #define DMA_HIFCR_CTCIF6_Pos     (21U)
 * #define DMA_HIFCR_CFEIF6_Pos     (16U)
 *
 * #define DMA_HIFCR_CTCIF5_Pos     (11U)
 * #define DMA_HIFCR_CFEIF5_Pos     (6U)
 *
 * #define DMA_HIFCR_CTCIF4_Pos     (5U)
 * #define DMA_HIFCR_CFEIF4_Pos     (0U)
 * -----------------------------------------------
 * #define DMA_LIFCR_CTCIF3_Pos     (27U)
 * #define DMA_LIFCR_CFEIF3_Pos     (22U)
 *
 * #define DMA_LIFCR_CTCIF2_Pos     (21U)
 * #define DMA_LIFCR_CFEIF2_Pos     (16U)
 *
 * #define DMA_LIFCR_CTCIF1_Pos     (11U)
 * #define DMA_LIFCR_CFEIF1_Pos     (6U)
 *
 * #define DMA_LIFCR_CTCIF0_Pos     (5U)
 * #define DMA_LIFCR_CFEIF0_Pos     (0U)
 *
 *  Bit 0: FEIF4 (流错误中断标志) —— 实际可能是“保留”或 FIFO 错误，不同型号位定义有差异
    Bit 1: 保留
    Bit 2: DMEIF4 (直接模式错误中断标志)
    Bit 3: TEIF4 (传输错误中断标志)
    Bit 4: HTIF4 (半传输中断标志)
    Bit 5: TCIF4 (传输完成中断标志)
 * */
void dma_clear_flag(const dma_stream_config_t *cfg) {
    xDMA_Base_TypeDef *base = (DMA_REQ_GET_CTRL(cfg->dma_request) == DMA_1) ? (xDMA_Base_TypeDef*)xDMA1_BASE :
                             (xDMA_Base_TypeDef*)xDMA2_BASE;
    // 清除 TC、HT、TE、DME、FE 标志 (bit5,4,3,2,0); 跳过保留位1
    //uint32_t mask = (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 0);
    // 也可以用 0x3D 来涵盖常见位 (bit5,4,3,2,0)
    uint32_t it_flags = 0x3F;
    uint8_t stream = DMA_REQ_GET_STREAM(cfg->dma_request);
    if (stream < 4) {
        uint32_t mask = it_flags << (stream * 6 + (stream >> 1) * 4); // 具体按手册，简化：清除对应流所有标志
        base->LIFCR |= mask;  // 写1清除
    } else {
        stream -= 4;
        uint32_t mask = it_flags << (stream * 6 + (stream >> 1) * 4);
        base->HIFCR |= mask;          // 写 1 清所有对应位
    }
}

nvic_irq_num dma_get_irqnum(const dma_stream_config_t *dma_conf) {
    if (DMA_REQ_GET_CTRL(dma_conf->dma_request) == DMA_1) {
        return DMA_REQ_GET_STREAM(dma_conf->dma_request) + DMA1_ST0_IRQ;
    } else {
        return DMA_REQ_GET_STREAM(dma_conf->dma_request)+ DMA2_ST0_IRQ;
    }
}
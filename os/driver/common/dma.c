/**
 * dma.c — DMA 管理逻辑（流分配、传输调度）
 * 所有寄存器操作通过 hal_dma_* 函数，本层只做管理逻辑。
 */
#include "dma.h"
#include "../../log/log.h"
#include "../../common/linear_pool.h"

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
    dma_controller_t ctrl = DMA_REQ_GET_CTRL(cfg->dma_request);
    uint8_t          strm = DMA_REQ_GET_STREAM(cfg->dma_request);

    if (ctrl >= xDMA_CONTROLLER_MAX || strm > 7) return DMA_ERROR;

    dma_stream_state_t *st = dma_states[ctrl][strm];
    if (!st) {
        st = os_malloc(sizeof(dma_stream_state_t));
        memset(st, 0, sizeof(dma_stream_state_t));
        dma_states[ctrl][strm] = st;
    }
    if (st->allocated) {
        LOG_ERROR("dma", "error dma %d stream %d have allocated", ctrl, strm);
        return DMA_ERROR;
    }

    hal_dma_clock_enable(ctrl);

    /* 构建 SxCR（包含中断使能位） */
    uint32_t cr = (DMA_REQ_GET_CHANNEL(cfg->dma_request) & 0x7) << 25;
    if (cfg->direction == DMA_DIR_P2M)      cr |= (0x0 << 6);
    else if (cfg->direction == DMA_DIR_M2P) cr |= (0x1 << 6);
    else                                     cr |= (0x2 << 6);
    cr |= (cfg->priority & 0x3) << 16;
    cr |= (cfg->mem_data_size & 0x3) << 13;
    cr |= (cfg->per_data_size & 0x3) << 11;
    if (cfg->mem_inc) cr |= (1 << 10);
    if (cfg->per_inc) cr |= (1 << 9);
    if (cfg->mode == DMA_MODE_CIRCULAR) cr |= (1 << 8);
    if (cfg->it_enable & xDMA_IT_TC)  cr |= (1 << 4);
    if (cfg->it_enable & xDMA_IT_HT)  cr |= (1 << 3);
    if (cfg->it_enable & xDMA_IT_TE)  cr |= (1 << 2);

    hal_dma_stream_write_cr(ctrl, strm, cr);

    /* FIFO 配置 */
    uint32_t fcr = 0;
    if (cfg->fifo_mode == DMA_FIFO_ENABLE) {
        fcr |= (1 << 2) | (0x3 << 0);
    }
    hal_dma_get_stream(ctrl, strm)->SxFCR = fcr;

    st->allocated = true;
    st->config = *cfg;
    LOG_DEBUG("dma", "dma init ctrl %d, stream %d", ctrl, strm);
    return DMA_SUCCESS;
}

int dma_stream_release(const dma_stream_config_t *cfg)
{
    dma_controller_t ctrl = DMA_REQ_GET_CTRL(cfg->dma_request);
    uint8_t          strm = DMA_REQ_GET_STREAM(cfg->dma_request);
    if (ctrl >= xDMA_CONTROLLER_MAX || strm > 7) return DMA_ERROR;

    dma_stream_state_t *st = dma_states[ctrl][strm];
    if (!st) return DMA_SUCCESS;

    st->allocated = false;
    hal_dma_stream_disable(ctrl, strm);
    hal_dma_stream_write_cr(ctrl, strm, 0);
    return DMA_SUCCESS;
}

int dma_start_transfer(const dma_stream_config_t *cfg,
                       uint32_t src_addr, uint32_t dst_addr, uint16_t count)
{
    dma_controller_t ctrl = DMA_REQ_GET_CTRL(cfg->dma_request);
    uint8_t          strm = DMA_REQ_GET_STREAM(cfg->dma_request);
    dma_stream_state_t *st = dma_states[ctrl][strm];
    if (!st || !st->allocated) return DMA_ERROR;

    hal_dma_stream_disable(ctrl, strm);
    hal_dma_clear_flags(ctrl, strm);
    hal_dma_stream_set_dir_addr(ctrl, strm, cfg->direction, src_addr, dst_addr);
    hal_dma_stream_set_ndtr(ctrl, strm, count);
    hal_dma_stream_enable(ctrl, strm);
    return DMA_SUCCESS;
}

int dma_stop_transfer(const dma_stream_config_t *cfg)
{
    dma_controller_t ctrl = DMA_REQ_GET_CTRL(cfg->dma_request);
    uint8_t          strm = DMA_REQ_GET_STREAM(cfg->dma_request);
    dma_stream_state_t *st = dma_states[ctrl][strm];
    if (!st || !st->allocated) return DMA_ERROR;

    hal_dma_stream_disable(ctrl, strm);
    hal_dma_clear_flags(ctrl, strm);
    return DMA_SUCCESS;
}

bool dma_is_busy(const dma_stream_config_t *cfg)
{
    dma_controller_t ctrl = DMA_REQ_GET_CTRL(cfg->dma_request);
    uint8_t          strm = DMA_REQ_GET_STREAM(cfg->dma_request);
    return hal_dma_is_busy(ctrl, strm);
}

void dma_clear_flag(const dma_stream_config_t *cfg)
{
    hal_dma_clear_flags(DMA_REQ_GET_CTRL(cfg->dma_request),
                        DMA_REQ_GET_STREAM(cfg->dma_request));
}

dma_it_event_t dma_get_it_event(const dma_stream_config_t *cfg)
{
    return hal_dma_get_it_event(DMA_REQ_GET_CTRL(cfg->dma_request),
                                DMA_REQ_GET_STREAM(cfg->dma_request));
}

nvic_irq_num dma_get_irqnum(const dma_stream_config_t *dma_conf) {
    if (DMA_REQ_GET_CTRL(dma_conf->dma_request) == DMA_1)
        return DMA_REQ_GET_STREAM(dma_conf->dma_request) + DMA1_ST0_IRQ;
    else
        return DMA_REQ_GET_STREAM(dma_conf->dma_request) + DMA2_ST0_IRQ;
}

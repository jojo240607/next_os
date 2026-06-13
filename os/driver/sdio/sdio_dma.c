/**
 * SDIO DMA 模式 — 步骤表状态机 + 中断驱动
 */
#include "sdio.h"
#include "../hal/hal_sdio.h"
#include "../common/dma.h"
#include "../../kernel/kworker.h"

static bool sdio_irq_handler(nvic_irq_t *irq_conf) {
    (void)irq_conf;
    gloable_kworker->wake->fun->give(gloable_kworker->wake);
    return true;
}

/* ════ 命令步骤表 ════ */
typedef struct { Device *dev; sdio_cmd_t *user_cmd; } cmd_ctx_t;
typedef void (*cmd_step_t)(kwork_t *w, cmd_ctx_t *c);
enum { CMD_STEP_SEND = 1, CMD_STEP_WAIT };

static void cmd_send(kwork_t *w, cmd_ctx_t *c) {
    sdio_cmd_t *cmd = c->user_cmd;
    hal_sdio_clear_icr(0xFFFFFFFF);
    hal_sdio_set_arg(cmd->arg);
    hal_sdio_send_cmd(hal_sdio_build_cmd(cmd->cmd, cmd->resp_type & 0x3));
    w->state = CMD_STEP_WAIT;
}
static void cmd_wait(kwork_t *w, cmd_ctx_t *c) {
    sdio_cmd_t *cmd = c->user_cmd;
    uint32_t sta = hal_sdio_get_sta();
    if (hal_sdio_sta_has_error(sta)) {
        cmd->error = -1;
    } else if (sta & xSDIO_STA_CMDREND) {
        hal_sdio_clear_icr(xSDIO_STA_CMDREND);
        if (cmd->resp_type != SDIO_RESPONSE_NO) {
            cmd->resp[0] = hal_sdio_get_resp(0);
            if (cmd->resp_type == SDIO_RESPONSE_LONG) {
                cmd->resp[1] = hal_sdio_get_resp(1);
                cmd->resp[2] = hal_sdio_get_resp(2);
                cmd->resp[3] = hal_sdio_get_resp(3);
            }
        }
        cmd->error = 0;
    } else {
        w->state = CMD_STEP_WAIT; return;
    }
    c->dev->fun->trigger_event(c->dev, SDIO_CMD_DONE, c->dev->arg);
    w->state = 0;
}
static const cmd_step_t cmd_steps[] = { cmd_send, cmd_wait };

/* ════ DMA 数据步骤表 ════ */
typedef struct {
    Device *dev; sdio_data_t data; uint32_t block_addr;
    uint8_t cmd_idx; int *result; uint32_t *p_block_addr; uint32_t block_size;
} data_ctx_t;
typedef void (*data_step_t)(kwork_t *w, data_ctx_t *c);
enum {
    DMA_STEP_SETUP = 1, DMA_STEP_CMD_DONE,
    DMA_STEP_START, DMA_STEP_WAIT_END,
};

static void dma_setup_cmd(kwork_t *w, data_ctx_t *c) {
    hal_sdio_set_dlen(c->data.len);
    hal_sdio_set_dtimer(0xFFFFFFFF);
    uint32_t dctrl = (c->data.block_size << 4);
    if (c->data.dir_to_card) dctrl |= (1UL << 0);
    dctrl |= (1UL << 1);
    hal_sdio_set_dctrl(dctrl);
    hal_sdio_clear_icr(0xFFFFFFFF);
    hal_sdio_set_arg(c->block_addr);
    hal_sdio_send_cmd(hal_sdio_build_cmd(c->cmd_idx, 1));
    w->state = DMA_STEP_CMD_DONE;
}
static void dma_cmd_done(kwork_t *w, data_ctx_t *c) {
    uint32_t sta = hal_sdio_get_sta();
    if (hal_sdio_sta_has_error(sta)) {
        *c->result = -1; c->data.error = -1;
        c->dev->fun->trigger_event(c->dev, SDIO_DATA_DONE, c->dev->arg);
        w->state = 0; return;
    }
    if (sta & xSDIO_STA_CMDREND) {
        hal_sdio_clear_icr(xSDIO_STA_CMDREND);
        w->state = DMA_STEP_START; return;
    }
    w->state = DMA_STEP_CMD_DONE;
}
static void dma_start(kwork_t *w, data_ctx_t *c) {
    const sdio_config_t *conf = c->dev->info->conf;
    if (c->data.dir_to_card && conf->dma_cfg->tx_dma)
        dma_start_transfer(conf->dma_cfg->tx_dma, (uint32_t)c->data.buf,
            hal_sdio_get_fifo_addr(), c->data.len / 4);
    else if (!c->data.dir_to_card && conf->dma_cfg->rx_dma)
        dma_start_transfer(conf->dma_cfg->rx_dma, hal_sdio_get_fifo_addr(),
            (uint32_t)c->data.buf, c->data.len / 4);
    w->state = DMA_STEP_WAIT_END;
}
static void dma_wait_end(kwork_t *w, data_ctx_t *c) {
    uint32_t sta = hal_sdio_get_sta();
    if (hal_sdio_sta_has_error(sta)) {
        *c->result = -1; c->data.error = -1;
        c->dev->fun->trigger_event(c->dev, SDIO_DATA_DONE, c->dev->arg);
        w->state = 0; return;
    }
    if (sta & xSDIO_STA_DATAEND) {
        hal_sdio_clear_icr(xSDIO_STA_DATAEND);
        *c->result = 0; *c->p_block_addr += c->data.len / c->block_size;
        w->state = 0; return;
    }
    w->state = DMA_STEP_WAIT_END;
}
static const data_step_t dma_steps[] = { dma_setup_cmd, dma_cmd_done, dma_start, dma_wait_end };

static int cmd_work_fn(kwork_t *w) {
    cmd_ctx_t *c = (cmd_ctx_t *)w->ctx;
    cmd_steps[w->state - 1](w, c);
    return w->state;
}
static int data_work_fn(kwork_t *w) {
    data_ctx_t *c = (data_ctx_t *)w->ctx;
    dma_steps[w->state - 1](w, c);
    return w->state;
}

void sdio_dma_dev_init(Device *self) {
    Sdio *sdio = GET_SDIO(self);
    if (!sdio->kwork) { sdio->kwork = os_malloc(sizeof(kwork_t)); memset(sdio->kwork, 0, sizeof(kwork_t)); }
    const sdio_config_t *conf = self->info->conf;
    if (conf->dma_cfg && conf->dma_cfg->tx_dma) dma_stream_request(conf->dma_cfg->tx_dma);
    if (conf->dma_cfg && conf->dma_cfg->rx_dma) dma_stream_request(conf->dma_cfg->rx_dma);
    if (conf->dma_cfg) hal_sdio_enable_dma();
    hal_sdio_set_mask(SDIO_IT_CMDREND | SDIO_IT_DATAEND |
                      SDIO_IT_CTIMEOUT | SDIO_IT_CCRCFAIL |
                      SDIO_IT_DTIMEOUT | SDIO_IT_DCRCFAIL);
    self->irq_conf->handler = sdio_irq_handler;
    self->irq_conf->arg = self;
    self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, SDIO_IRQ);
    self->fun->config_irq(self, self->irq_conf);
}

void sdio_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Sdio *sdio = GET_SDIO(self);
    switch (cmd) {
    case SDIO_IOCTL_SEND_CMD:
        if (arg) {
            cmd_ctx_t ctx = { .dev = self, .user_cmd = (sdio_cmd_t *)arg };
            kwork_init(sdio->kwork, cmd_work_fn, &ctx);
            kwork_submit(gloable_kworker, sdio->kwork);
            self->fun->trigger_event(self, SDIO_XFER_START, self->arg);
        }
        break;
    case SDIO_IOCTL_SET_BLOCK: if (arg) sdio->block_addr = *(uint32_t *)arg; break;
    default: break;
    }
}
size_t sdio_dma_read(Device *self, void *buf, size_t count) {
    Sdio *sdio = GET_SDIO(self); int result = -1;
    data_ctx_t ctx = { .dev = self, .block_addr = sdio->block_addr, .cmd_idx = 17,
        .data = { .buf = buf, .len = (uint32_t)count, .block_size = sdio->block_size, .dir_to_card = false },
        .result = &result, .p_block_addr = &sdio->block_addr, .block_size = sdio->block_size };
    kwork_init(sdio->kwork, data_work_fn, &ctx);
    kwork_submit(gloable_kworker, sdio->kwork);
    self->fun->trigger_event(self, SDIO_XFER_START, self->arg);
    return result == 0 ? count : 0;
}
void sdio_dma_write(Device *self, const void *buf, size_t count) {
    Sdio *sdio = GET_SDIO(self); int result = -1;
    data_ctx_t ctx = { .dev = self, .block_addr = sdio->block_addr, .cmd_idx = 24,
        .data = { .buf = (uint8_t *)buf, .len = (uint32_t)count, .block_size = sdio->block_size, .dir_to_card = true },
        .result = &result, .p_block_addr = &sdio->block_addr, .block_size = sdio->block_size };
    kwork_init(sdio->kwork, data_work_fn, &ctx);
    kwork_submit(gloable_kworker, sdio->kwork);
    self->fun->trigger_event(self, SDIO_XFER_START, self->arg);
}

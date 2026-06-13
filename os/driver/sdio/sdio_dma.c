/**
 * sdio_dma.c — SDIO 总线 DMA 模式 (kwork 状态机驱动)
 * 提供非阻塞命令发送和数据传输，不绑定 SD Card 协议。
 */
#include "sdio.h"
#include "../hal/hal_sdio.h"
#include "../common/dma.h"
#include "../../kernel/kwork.h"
#include "../../kernel/kworker.h"
#include "../../common/linear_pool.h"

/* ── ISR: 唤醒 kworker ── */
static bool sdio_irq_handler(nvic_irq_t *irq_conf) {
    (void)irq_conf;
    gloable_kworker->wake->fun->give(gloable_kworker->wake);
    return true;
}

/* ════ 命令状态机 ════ */
typedef struct { Device *dev; sdio_cmd_t *user_cmd; } cmd_ctx_t;
typedef void (*cmd_step_t)(kwork_t *w, cmd_ctx_t *c);
enum { CMD_STEP_SEND = 1, CMD_STEP_WAIT };

static void cmd_send(kwork_t *w, cmd_ctx_t *c) {
    hal_sdio_clear_icr(0xFFFFFFFF);
    hal_sdio_set_arg(c->user_cmd->arg);
    hal_sdio_send_cmd(hal_sdio_build_cmd(c->user_cmd->cmd, c->user_cmd->resp_type & 0x3));
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
    } else { w->state = CMD_STEP_WAIT; return; }
    c->dev->fun->trigger_event(c->dev, SDIO_CMD_DONE, NULL);
    w->state = 0;
}
static const cmd_step_t cmd_steps[] = { cmd_send, cmd_wait };

/* ════ DMA 数据状态机 ════ */
typedef struct {
    Device *dev; sdio_cmd_t *cmd; sdio_data_t *data; int *result;
} data_ctx_t;
typedef void (*data_step_t)(kwork_t *w, data_ctx_t *c);
enum { DMA_S_SETUP = 1, DMA_S_CMD_DONE, DMA_S_DMA_START, DMA_S_WAIT_END };

static void dma_setup_cmd(kwork_t *w, data_ctx_t *c) {
    hal_sdio_set_dlen(c->data->len);
    hal_sdio_set_dtimer(0xFFFFFFFF);
    uint32_t dctrl = (c->data->block_size << 4);
    if (c->data->dir_to_card) dctrl |= (1UL << 0);
    dctrl |= (1UL << 1);
    hal_sdio_set_dctrl(dctrl);
    hal_sdio_clear_icr(0xFFFFFFFF);
    hal_sdio_set_arg(c->cmd->arg);
    hal_sdio_send_cmd(hal_sdio_build_cmd(c->cmd->cmd, c->cmd->resp_type));
    w->state = DMA_S_CMD_DONE;
}
static void dma_cmd_done(kwork_t *w, data_ctx_t *c) {
    uint32_t sta = hal_sdio_get_sta();
    if (hal_sdio_sta_has_error(sta)) { *c->result = -1; w->state = 0; return; }
    if (sta & xSDIO_STA_CMDREND) { hal_sdio_clear_icr(xSDIO_STA_CMDREND); w->state = DMA_S_DMA_START; return; }
    w->state = DMA_S_CMD_DONE;
}
static void dma_start_xfer(kwork_t *w, data_ctx_t *c) {
    const sdio_config_t *conf = c->dev->info->conf;
    if (c->data->dir_to_card && conf->dma_cfg && conf->dma_cfg->tx_dma)
        dma_start_transfer(conf->dma_cfg->tx_dma, (uint32_t)c->data->buf,
            hal_sdio_get_fifo_addr(), c->data->len / 4);
    else if (!c->data->dir_to_card && conf->dma_cfg && conf->dma_cfg->rx_dma)
        dma_start_transfer(conf->dma_cfg->rx_dma, hal_sdio_get_fifo_addr(),
            (uint32_t)c->data->buf, c->data->len / 4);
    w->state = DMA_S_WAIT_END;
}
static void dma_wait_end(kwork_t *w, data_ctx_t *c) {
    uint32_t sta = hal_sdio_get_sta();
    if (hal_sdio_sta_has_error(sta)) { *c->result = -1; c->data->error = -1; w->state = 0; return; }
    if (sta & xSDIO_STA_DATAEND) { hal_sdio_clear_icr(xSDIO_STA_DATAEND); *c->result = 0; w->state = 0; return; }
    w->state = DMA_S_WAIT_END;
}
static const data_step_t dma_steps[] = { dma_setup_cmd, dma_cmd_done, dma_start_xfer, dma_wait_end };

/* ── work 函数 ── */
static int cmd_work_fn(kwork_t *w) { cmd_ctx_t *c = (cmd_ctx_t *)w->ctx; cmd_steps[w->state-1](w,c); return w->state; }
static int data_work_fn(kwork_t *w){ data_ctx_t *c=(data_ctx_t*)w->ctx; dma_steps[w->state-1](w,c); return w->state; }

/* ── 驱动初始化 ── */
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

/* ── ioctl: 发命令或启动数据传输 ── */
void sdio_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Sdio *sdio = GET_SDIO(self);
    switch (cmd) {
    case SDIO_CMD_SEND:
        if (arg) {
            cmd_ctx_t ctx = { .dev = self, .user_cmd = (sdio_cmd_t *)arg };
            kwork_init((kwork_t*)sdio->kwork, cmd_work_fn, &ctx);
            kwork_submit(gloable_kworker, (kwork_t*)sdio->kwork);
            self->fun->trigger_event(self, SDIO_XFER_START, NULL);
        }
        break;
    case SDIO_DATA_XFER:
        if (arg) {
            ((sdio_data_t*)arg)->error = -1; int result = -1;
            data_ctx_t ctx = { .dev = self, .cmd = NULL, .data = (sdio_data_t*)arg, .result = &result };
            kwork_init((kwork_t*)sdio->kwork, data_work_fn, &ctx);
            kwork_submit(gloable_kworker, (kwork_t*)sdio->kwork);
            self->fun->trigger_event(self, SDIO_XFER_START, NULL);
        }
        break;
    default: break;
    }
}

size_t sdio_dma_read(Device *self, void *buf, size_t count)  { (void)self;(void)buf;(void)count; return 0; }
void   sdio_dma_write(Device *self, const void *buf, size_t count) { (void)self;(void)buf;(void)count; }

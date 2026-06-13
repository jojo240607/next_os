/**
 * sdio_poll.c — SDIO 总线轮询模式
 * 提供阻塞式命令发送和数据 FIFO 搬运，不绑定任何上层协议。
 */
#include "sdio.h"
#include "../hal/hal_sdio.h"

/* ── 阻塞发送一条命令 ── */
static int sdio_poll_send_cmd(sdio_cmd_t *cmd) {
    uint32_t timeout = 0xFFFFF;

    /* 等待总线空闲 */
    while (!hal_sdio_sta_has_cmd_done(hal_sdio_get_sta()))
        if (--timeout == 0) { cmd->error = -1; return -1; }

    hal_sdio_clear_icr(0xFFFFFFFF);
    hal_sdio_set_arg(cmd->arg);
    hal_sdio_send_cmd(hal_sdio_build_cmd(cmd->cmd, cmd->resp_type & 0x3));

    timeout = 0xFFFFF;
    while (1) {
        uint32_t sta = hal_sdio_get_sta();
        if (hal_sdio_sta_has_error(sta)) { cmd->error = -1; return -1; }
        if (hal_sdio_sta_has_cmd_done(sta)) break;
        if (--timeout == 0) { cmd->error = -1; return -1; }
    }

    if (cmd->resp_type != SDIO_RESPONSE_NO) {
        cmd->resp[0] = hal_sdio_get_resp(0);
        if (cmd->resp_type == SDIO_RESPONSE_LONG) {
            cmd->resp[1] = hal_sdio_get_resp(1);
            cmd->resp[2] = hal_sdio_get_resp(2);
            cmd->resp[3] = hal_sdio_get_resp(3);
        }
    }
    cmd->error = 0;
    return 0;
}

/* ── 阻塞式 FIFO 数据搬运 ── */
static int sdio_poll_xfer_data(sdio_data_t *data) {
    uint32_t words = data->len / 4, timeout;

    if (data->dir_to_card) {
        for (uint32_t i = 0; i < words; i++) {
            timeout = 0xFFFFF;
            while (!hal_sdio_sta_check_txfifohe())
                if (--timeout == 0) return -1;
            hal_sdio_write_fifo(((uint32_t *)data->buf)[i]);
        }
    } else {
        for (uint32_t i = 0; i < words; i++) {
            timeout = 0xFFFFF;
            while (!hal_sdio_sta_check_rxfifohf())
                if (--timeout == 0) return -1;
            ((uint32_t *)data->buf)[i] = hal_sdio_read_fifo();
        }
    }
    timeout = 0xFFFFFF;
    while (!hal_sdio_sta_check_dataend())
        if (--timeout == 0) return -1;
    data->error = 0;
    return 0;
}

/* ── setup 数据通道 + 发命令 + FIFO 搬运 ── */
static int sdio_poll_xfer_setup(sdio_cmd_t *cmd, sdio_data_t *data) {
    hal_sdio_set_dlen(data->len);
    hal_sdio_set_dtimer(0xFFFFFFFF);
    uint32_t dctrl = (data->block_size << 4);
    if (data->dir_to_card) dctrl |= (1UL << 0);
    dctrl |= (1UL << 1);
    hal_sdio_set_dctrl(dctrl);

    if (sdio_poll_send_cmd(cmd) < 0) return -1;
    return sdio_poll_xfer_data(data);
}

/* ── 公开 API: 阻塞式命令+数据传输 (供上层协议层使用) ── */
int sdio_poll_block_xfer(Sdio *sdio, sdio_cmd_t *cmd, sdio_data_t *data) {
    (void)sdio;  /* poll 模式无需 Sdio 上下文 */
    return sdio_poll_xfer_setup(cmd, data);
}

/* ── Device VTable ── */
void sdio_poll_dev_init(Device *self) { (void)self; }

void sdio_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    switch (cmd) {
    case SDIO_CMD_SEND:
        if (arg) sdio_poll_send_cmd((sdio_cmd_t *)arg);
        break;
    default: break;
    }
}

size_t sdio_poll_read(Device *self, void *buf, size_t count) {
    /* 便捷接口：从 sdio_data_t 中读取 */
    (void)self; (void)buf; (void)count;
    return 0; /* 建议通过 ioctl 使用 */
}

void sdio_poll_write(Device *self, const void *buf, size_t count) {
    (void)self; (void)buf; (void)count;
    /* 建议通过 ioctl 使用 */
}

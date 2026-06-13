/**
 * SDIO 轮询模式
 */
#include "sdio.h"
#include "../hal/hal_sdio.h"

/* ── 阻塞发送命令 (轮询) ── */
static int sdio_send_cmd(sdio_cmd_t *cmd) {
    uint32_t timeout = 0xFFFFF;
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

void sdio_poll_dev_init(Device *self) { (void)self; }

void sdio_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Sdio *sdio = GET_SDIO(self);
    switch (cmd) {
    case SDIO_IOCTL_SEND_CMD: if (arg) sdio_send_cmd((sdio_cmd_t *)arg); break;
    case SDIO_IOCTL_SET_BLOCK: if (arg) sdio->block_addr = *(uint32_t *)arg; break;
    default: break;
    }
}

/* ── 轮询 FIFO 数据搬运 ── */
static int sdio_data_xfer(sdio_data_t *data) {
    uint32_t words = data->len / 4, timeout;
    if (data->dir_to_card) {
        for (uint32_t i = 0; i < words; i++) {
            timeout = 0xFFFFF;
            while (!hal_sdio_sta_check_txfifohe()) { if (--timeout == 0) return -1; }
            hal_sdio_write_fifo(((uint32_t *)data->buf)[i]);
        }
    } else {
        for (uint32_t i = 0; i < words; i++) {
            timeout = 0xFFFFF;
            while (!hal_sdio_sta_check_rxfifohf()) { if (--timeout == 0) return -1; }
            ((uint32_t *)data->buf)[i] = hal_sdio_read_fifo();
        }
    }
    timeout = 0xFFFFFF;
    while (!hal_sdio_sta_check_dataend()) { if (--timeout == 0) return -1; }
    data->error = 0;
    return 0;
}

static int data_transfer(Sdio *sdio, uint8_t cmd_idx, sdio_data_t *data) {
    hal_sdio_set_dlen(data->len);
    hal_sdio_set_dtimer(0xFFFFFFFF);
    uint32_t dctrl = (data->block_size << 4);
    if (data->dir_to_card) dctrl |= (1UL << 0);
    dctrl |= (1UL << 1);
    hal_sdio_set_dctrl(dctrl);

    sdio_cmd_t cmd = { .cmd = cmd_idx, .arg = sdio->block_addr,
                       .resp_type = SDIO_RESPONSE_SHORT, .error = 0 };
    if (sdio_send_cmd(&cmd) < 0) return -1;
    return sdio_data_xfer(data);
}

size_t sdio_poll_read(Device *self, void *buf, size_t count) {
    Sdio *sdio = GET_SDIO(self);
    sdio_data_t data = { .buf = buf, .len = (uint32_t)count,
                         .block_size = sdio->block_size, .dir_to_card = false };
    if (data_transfer(sdio, 17, &data) < 0) return 0;
    sdio->block_addr += count / sdio->block_size;
    return count;
}

void sdio_poll_write(Device *self, const void *buf, size_t count) {
    Sdio *sdio = GET_SDIO(self);
    sdio_data_t data = { .buf = (uint8_t *)buf, .len = (uint32_t)count,
                         .block_size = sdio->block_size, .dir_to_card = true };
    if (data_transfer(sdio, 24, &data) == 0)
        sdio->block_addr += count / sdio->block_size;
}

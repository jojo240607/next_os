/**
 * sd_card.c — SD Memory Card 协议层
 *
 * 通过 SDIO 总线层（sdio_send_cmd / sdio_poll_block_xfer）操作 SD 卡。
 * 不直接访问任何硬件寄存器。
 */
#include "sd_card.h"
#include "../hal/hal_sdio.h"
#include "../common/device.h"

/* ── SDIO 总线 API（模式特定） ── */
extern int  sdio_poll_block_xfer(Sdio *sdio, sdio_cmd_t *cmd, sdio_data_t *data);

/* ── 通过 dev_ioctl 发送命令 (所有模式通用) ── */
static int sdio_send_cmd(Sdio *sdio, sdio_cmd_t *cmd) {
    GET_DEVICE(sdio)->vtable->dev_ioctl(GET_DEVICE(sdio), SDIO_CMD_SEND, cmd);
    return cmd->error;
}

/* ═══════════════════ SD 卡初始化 ═══════════════════ */

int sd_card_init(Sdio *sdio, sd_card_info_t *info) {
    sdio_cmd_t cmd;
    uint32_t  resp;

    (void)info;  /* 初始化时 info 可能为 NULL */

    /* ─ 1. CMD0: GO_IDLE_STATE ─ */
    cmd = (sdio_cmd_t){ .cmd = 0, .arg = 0, .resp_type = SDIO_RESPONSE_NO };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;

    /* ─ 2. CMD8: SEND_IF_COND (电压 2.7-3.6V, 模式 0xAA) ─ */
    cmd = (sdio_cmd_t){ .cmd = 8, .arg = 0x000001AA, .resp_type = SDIO_RESPONSE_SHORT };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;
    /* CMD8 响应中 bit[11:8]=1 表示 2.7-3.6V 支持, bit[7:0]=0xAA */
    if ((cmd.resp[0] & 0xFFF) != 0x1AA) return -1;

    /* ─ 3. ACMD41: SD_SEND_OP_COND (等待卡就绪) ─ */
    uint32_t acmd41_arg = 0x40000000;  /* HCS=1: 支持 SDHC */
    for (int retry = 0; retry < 1000; retry++) {
        /* CMD55: APP_CMD (前缀) */
        cmd = (sdio_cmd_t){ .cmd = 55, .arg = 0, .resp_type = SDIO_RESPONSE_SHORT };
        if (sdio_send_cmd(sdio, &cmd) < 0) return -1;

        /* ACMD41 */
        cmd = (sdio_cmd_t){ .cmd = 41, .arg = acmd41_arg, .resp_type = SDIO_RESPONSE_SHORT };
        if (sdio_send_cmd(sdio, &cmd) < 0) return -1;
        resp = cmd.resp[0];

        if (resp & (1UL << 31)) break;  /* busy=0 → 卡就绪 */
        /* 轮询延时 */
        for (volatile int d = 0; d < 10000; d++) __NOP();
    }
    if (!(resp & (1UL << 31))) return -1;  /* 超时 */

    /* 判断卡类型 */
    uint8_t card_type = (resp & (1UL << 30)) ? 1 : 0;  /* CCS=1 → SDHC/SDXC */

    /* ─ 4. CMD2: ALL_SEND_CID ─ */
    cmd = (sdio_cmd_t){ .cmd = 2, .arg = 0, .resp_type = SDIO_RESPONSE_LONG };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;
    /* resp[0..3] 包含 128-bit CID, 暂不解析 */

    /* ─ 5. CMD3: SEND_RELATIVE_ADDR ─ */
    cmd = (sdio_cmd_t){ .cmd = 3, .arg = 0, .resp_type = SDIO_RESPONSE_SHORT };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;
    uint16_t rca = (cmd.resp[0] >> 16) & 0xFFFF;

    /* ─ 6. CMD7: SELECT_CARD ─ */
    cmd = (sdio_cmd_t){ .cmd = 7, .arg = ((uint32_t)rca << 16), .resp_type = SDIO_RESPONSE_SHORT };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;

    /* ─ 7. ACMD6: SET_BUS_WIDTH (切换到 4-bit) ─ */
    cmd = (sdio_cmd_t){ .cmd = 55, .arg = ((uint32_t)rca << 16), .resp_type = SDIO_RESPONSE_SHORT };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;
    cmd = (sdio_cmd_t){ .cmd = 6, .arg = 2, .resp_type = SDIO_RESPONSE_SHORT };  /* arg=2 → 4-bit */
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;

    /* ─ 8. CMD9: SEND_CSD (读取卡参数) ─ */
    cmd = (sdio_cmd_t){ .cmd = 9, .arg = ((uint32_t)rca << 16), .resp_type = SDIO_RESPONSE_LONG };
    if (sdio_send_cmd(sdio, &cmd) < 0) return -1;

    uint32_t csd0 = cmd.resp[0];
    uint32_t csd1 = cmd.resp[1];

    /* 解析 CSD 获取容量和块大小 */
    uint32_t c_size, c_size_mul, read_bl_len;
    read_bl_len = (csd1 >> 16) & 0xF;       /* READ_BL_LEN */
    uint32_t block_size = 1UL << read_bl_len;

    if ((csd0 >> 30) == 0) {  /* CSD v1.0 (SDSC) */
        c_size     = ((csd0 >> 8) & 0x3) << 10 | (csd0 & 0xFF) << 2 | (csd1 >> 30);
        c_size_mul = (csd1 >> 15) & 0x7;
        uint32_t capacity = (c_size + 1) * (1UL << (c_size_mul + 2)) * block_size / 1024 / 1024;
        if (info) { info->capacity_mb = capacity; info->block_size = block_size; }
    } else {  /* CSD v2.0 (SDHC/SDXC) */
        c_size = ((csd0 >> 8) & 0x3F) << 16 | (csd0 & 0xFF) << 8 | (csd1 >> 24);
        uint32_t capacity = (c_size + 1) * 512 / 1024;  /* KB */
        if (info) {
            info->capacity_mb = capacity / 1024;
            info->block_size  = 512;
        }
    }
    if (info) {
        info->block_count = info->capacity_mb * 1024 * 1024 / info->block_size;
        info->card_type   = card_type;
        info->rca         = rca;
        info->bus_width   = 4;
    }

    /* ─ 设置高速时钟 (卡初始化完成后) ─ */
    const sdio_config_t *conf = GET_DEVICE(sdio)->info->conf;
    if (conf) {
        uint32_t clkcr = hal_sdio_build_clkcr(0, SDIO_BUS_WIDTH_4,
            conf->clock_edge, conf->power_save, conf->hw_flow_control);
        hal_sdio_config_clock(clkcr);
    }

    return 0;
}

/* ═══════════════════ 块读写 ═══════════════════ */

int sd_card_read_block(Sdio *sdio, uint32_t addr, uint8_t *buf) {
    uint8_t block_buf[512];
    sdio_cmd_t  cmd  = { .cmd = 17, .arg = addr, .resp_type = SDIO_RESPONSE_SHORT };
    sdio_data_t data = { .buf = block_buf, .len = 512, .block_size = 512,
                         .dir_to_card = false };

    if (sdio_poll_block_xfer(sdio, &cmd, &data) < 0) return -1;
    /* 拷贝结果到用户 buffer */
    for (int i = 0; i < 512; i++) buf[i] = block_buf[i];
    return 0;
}

int sd_card_write_block(Sdio *sdio, uint32_t addr, const uint8_t *buf) {
    uint8_t block_buf[512];
    for (int i = 0; i < 512; i++) block_buf[i] = buf[i];

    sdio_cmd_t  cmd  = { .cmd = 24, .arg = addr, .resp_type = SDIO_RESPONSE_SHORT };
    sdio_data_t data = { .buf = block_buf, .len = 512, .block_size = 512,
                         .dir_to_card = true };

    return sdio_poll_block_xfer(sdio, &cmd, &data);
}

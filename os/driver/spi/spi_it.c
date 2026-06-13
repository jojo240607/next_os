/**
 * SPI 中断模式
 */
#include "spi.h"
#include "../hal/hal_spi.h"
#include "../../common/linear_pool.h"

/* ── 中断处理 ── */
static bool spi_irq_handler(nvic_irq_t *irq_conf) {
    Spi *spi = (Spi *)irq_conf->arg;
    const spi_config_t *conf = GET_DEVICE(spi)->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    uint16_t sr = hal_spi_get_it_event(conf->id);

    if (sr & (0x07 << 4)) {
        if (sr & (1 << 6)) { volatile uint8_t d __attribute__((unused)) = *hal_spi_data_addr(conf->id); (void)d; }
        if (sr & (1 << 5)) { hal_spi_disable(conf->id); hal_spi_enable(conf->id); }
    }

    if (sr & xSPI_IT_RXNE) {
        uint8_t data = *hal_spi_data_addr(conf->id);
        if (x && x->rx_user_buf->buf) {
            x->rx_user_buf->buf[x->rx_user_buf->pos++] = data;
        }
    }

    if (sr & xSPI_IT_TXE) {
        if (x && x->tx_user_buf->pos < x->tx_user_buf->buf_size) {
            uint8_t byte = x->tx_user_buf->buf ? x->tx_user_buf->buf[x->tx_user_buf->pos] : 0xFF;
            *hal_spi_data_addr(conf->id) = byte;
            x->tx_user_buf->pos++;
        } else {
            hal_spi_clear_it(conf->id, xSPI_IE_TXE);
        }
    }

    if (x && x->tx_user_buf->pos >= x->tx_user_buf->buf_size
        && x->rx_user_buf->pos >= x->rx_user_buf->buf_size) {
        x->active = false;
        hal_spi_clear_it(conf->id, xSPI_IE_RXNE | xSPI_IE_TXE);
        if (x->cs_pin) gpio_set(x->cs_pin);
        else           hal_spi_disable(conf->id);
        GET_DEVICE(spi)->fun->trigger_event(GET_DEVICE(spi), SPI_XFER_DONE, GET_DEVICE(spi)->arg);
    }
    return true;
}

/* ── dev_init ── */
void spi_it_dev_init(Device *self) {
    const spi_config_t *conf = self->info->conf;
    Spi *spi = GET_SPI(self);

    if (!spi->spi_xfer) {
        spi->spi_xfer = os_malloc(sizeof(spi_xfer_t));
        memset(spi->spi_xfer, 0, sizeof(spi_xfer_t));
        spi->spi_xfer->tx_user_buf = os_malloc(sizeof(spi_cache_t));
        memset(spi->spi_xfer->tx_user_buf, 0, sizeof(spi_cache_t));
        spi->spi_xfer->rx_user_buf = os_malloc(sizeof(spi_cache_t));
        memset(spi->spi_xfer->rx_user_buf, 0, sizeof(spi_cache_t));
    }

    hal_spi_it_init(conf->id, conf->it_enable);
    self->irq_conf->handler = spi_irq_handler;
    self->irq_conf->arg = self;
    self->irq_conf->irq_list->fun->add_int(
        self->irq_conf->irq_list, (int)(SPI1_IRQ + conf->id));
    self->fun->config_irq(self, self->irq_conf);
}

/* ── ioctl (IT) ── */
void spi_it_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    if (cmd == DEVICE_TRANSFER && arg) {
        spi_transfer_args_t *a = (spi_transfer_args_t *)arg;
        const gpio_t *cs = a->cs_pin ? a->cs_pin : spi->cs_pin;

        if (!x || x->active) return;
        if (cs) gpio_reset(cs);
        else    hal_spi_enable(conf->id);

        x->cs_pin              = cs;
        x->rx_user_buf->buf    = a->rx_buf;
        x->rx_user_buf->buf_size = a->len;
        x->rx_user_buf->pos    = 0;
        x->tx_user_buf->buf    = (uint8_t *)a->tx_buf;
        x->tx_user_buf->buf_size = a->len;
        x->tx_user_buf->pos    = 0;
        x->active              = true;

        hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);
        uint8_t first = a->tx_buf ? a->tx_buf[0] : 0xFF;
        *hal_spi_data_addr(conf->id) = first;
        x->tx_user_buf->pos = 1;
        self->fun->trigger_event(self, SPI_XFER_START, self->arg);
    }
}

/* ── 读写 ── */
size_t spi_it_read(Device *self, void *buf, size_t count) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    if (!x || x->active) return 0;

    if (spi->cs_pin) gpio_reset(spi->cs_pin);
    x->cs_pin              = spi->cs_pin;
    x->rx_user_buf->buf      = buf;
    x->rx_user_buf->buf_size = (uint16_t)count;
    x->rx_user_buf->pos      = 0;
    x->tx_user_buf->buf      = NULL;
    x->tx_user_buf->buf_size = 0;
    x->tx_user_buf->pos      = 0;
    x->active                = true;

    hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);
    *hal_spi_data_addr(conf->id) = 0xFF;
    x->tx_user_buf->pos = 1;
    self->fun->trigger_event(self, SPI_XFER_START, self->arg);
    return count;
}

void spi_it_write(Device *self, const void *buf, size_t count) {
    Spi *spi = GET_SPI(self);
    const spi_config_t *conf = self->info->conf;
    spi_xfer_t *x = spi->spi_xfer;
    if (!x || x->active) return;

    if (spi->cs_pin) gpio_reset(spi->cs_pin);
    x->cs_pin              = spi->cs_pin;
    x->tx_user_buf->buf      = (uint8_t *)buf;
    x->tx_user_buf->buf_size = (uint16_t)count;
    x->tx_user_buf->pos      = 0;
    x->rx_user_buf->buf      = NULL;
    x->rx_user_buf->buf_size = 0;
    x->rx_user_buf->pos      = 0;
    x->active                = true;

    hal_spi_set_it(conf->id, xSPI_IE_TXE | xSPI_IE_RXNE);
    uint8_t first = buf ? *(uint8_t *)buf : 0xFF;
    *hal_spi_data_addr(conf->id) = first;
    x->tx_user_buf->pos = 1;
    self->fun->trigger_event(self, SPI_XFER_START, self->arg);
}

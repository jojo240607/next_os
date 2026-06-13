#include "fsmc.h"
#include "../common/dma.h"

static bool fsmc_dma_irq_handler(nvic_irq_t *irq_conf) {
    Device *dev = (Device *)irq_conf->arg;
    Fsmc *fsmc = GET_FSMC(dev);
    dma_clear_flag(((fsmc_config_t *)dev->info->conf)->dma_cfg);
    dev->fun->trigger_event(dev, FSMC_XFER_DONE, dev->arg);
    return true;
}

void fsmc_dma_dev_init(Device *self) {
    const fsmc_config_t *conf = self->info->conf;
    if (!conf->dma_cfg) return;
    dma_stream_request(conf->dma_cfg);
    if (conf->dma_cfg->it_enable) {
        self->irq_conf->handler = fsmc_dma_irq_handler;
        self->irq_conf->arg = self;
        self->irq_conf->irq_list->fun->add_int(
                self->irq_conf->irq_list, dma_get_irqnum(conf->dma_cfg));
        self->fun->config_irq(self, self->irq_conf);
    }
}

size_t fsmc_dma_read(Device *self, void *buf, size_t count) {
    Fsmc *fsmc = GET_FSMC(self);
    uint16_t *dst = (uint16_t *)buf;
    for (size_t i = 0; i < count / 2; i++) {
        dst[i] = *(volatile uint16_t *)fsmc->current_addr;
        if (fsmc->auto_inc) fsmc->current_addr += 2;
    }
    return count;
}

void fsmc_dma_write(Device *self, const void *buf, size_t count) {
    Fsmc *fsmc = GET_FSMC(self);
    const fsmc_config_t *conf = self->info->conf;
    dma_start_transfer(conf->dma_cfg, (uint32_t)buf, fsmc->current_addr, (uint16_t)(count / 2));
    if (fsmc->auto_inc) fsmc->current_addr += count;
    self->fun->trigger_event(self, FSMC_XFER_START, self->arg);
}

void fsmc_dma_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
    Fsmc *fsmc = GET_FSMC(self);
    switch (cmd) {
    case FSMC_IOCTL_SET_ADDR:
        fsmc->current_addr = FSMC_BANK1_BASE + (uint32_t)(uintptr_t)arg;
        break;
    case FSMC_IOCTL_SET_AUTOINC:
        fsmc->auto_inc = (bool)(uintptr_t)arg;
        break;
    default: break;
    }
}

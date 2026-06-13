#include "fsmc.h"

void fsmc_poll_dev_init(Device *self) { (void)self; }

size_t fsmc_poll_read(Device *self, void *buf, size_t count) {
    Fsmc *fsmc = GET_FSMC(self);
    uint16_t *dst = (uint16_t *)buf;
    for (size_t i = 0; i < count / 2; i++) {
        dst[i] = *(volatile uint16_t *)fsmc->current_addr;
        if (fsmc->auto_inc) fsmc->current_addr += 2;
    }
    return count;
}

void fsmc_poll_write(Device *self, const void *buf, size_t count) {
    Fsmc *fsmc = GET_FSMC(self);
    const uint16_t *src = (const uint16_t *)buf;
    for (size_t i = 0; i < count / 2; i++) {
        *(volatile uint16_t *)fsmc->current_addr = src[i];
        if (fsmc->auto_inc) fsmc->current_addr += 2;
    }
}

void fsmc_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) {
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

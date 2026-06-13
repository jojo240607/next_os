#include "hal_fsmc.h"
#include "../common/rcc.h"

void hal_fsmc_clock_enable(void) {
    rcc_periph_clock_enable(RCC_BUS_AHB3, xRCC_AHB3ENR_FSMCEN);
}

void hal_fsmc_config_bank1(uint32_t bcr, uint32_t btr) {
    xFSMC_Bank1[0].BCR = bcr;
    xFSMC_Bank1[0].BTR = btr;
}

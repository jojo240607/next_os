#include "rcc.h"
#include "cmsis_gcc.h"
#include "../hal/hal_rcc.h"
#include "../../log/log.h"


bool rcc_sysclk_init(const rcc_sysclk_config_t *cfg) {
    hal_rcc_sysclk_init(cfg);
}
void rcc_periph_clock_enable(rcc_bus_t bus, rcc_enter_t periph_bit) {
    hal_rcc_periph_clock_enable(bus, periph_bit);
}
void rcc_periph_clock_disable(rcc_bus_t bus, rcc_enter_t periph_bit) {
    hal_rcc_periph_clock_disable(bus, periph_bit);
}

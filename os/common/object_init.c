#include "object_init.h"
#include "linear_pool.h"
#include "../log/log.h"
#include "sys_time.h"
#include "sys_mutex.h"
#include "../driver/common/pinmux.h"

void all_object_init() {

    os_pool_init();
    log_init();
    LOG_DEBUG("obj_init", "all_object_init");
    systime_init();
    sys_mutex_init();
    pinmux_init();
}

#include "object_init.h"
#include "linear_pool.h"
#include "../log/log.h"

void all_object_init() {
    os_pool_init();
    log_init();
}

#include "wdg.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "hal/hal_wdg.h"

static void wdg_iwdg_reload();
static bool wdg_iwdg_reset_flag();
static void wdg_iwdg_clear();
static uint32_t wdg_iwdg_timeout(Wdg* self);

dev_init_override(wdg_dev_init_impl);

// 析构函数声明
static void wdg_destroy(Wdg* self);

// TODO: 初始化数据成员
static const WdgFun wdg_fun = {
    .destroy = wdg_destroy,
	.iwdg_reload = wdg_iwdg_reload,
	.iwdg_reset_flag = wdg_iwdg_reset_flag,
	.iwdg_clear = wdg_iwdg_clear,
	.iwdg_timeout = wdg_iwdg_timeout,
};


// 构造函数实现
Wdg* wdg_create(const iwdg_config_t *cfg, const dev_pripority_t *priority) {
    Wdg* obj = (Wdg*)os_malloc(sizeof(Wdg));
    if (obj) {
        memset(obj, 0, sizeof(Wdg));
        wdg_init(obj, cfg, priority);
    }
    return obj;
}

void wdg_init(Wdg* self, const iwdg_config_t *cfg, const dev_pripority_t *priority) {
    // 初始化基类部分
    device_init(&self->base, priority);
    self->fun = &(wdg_fun);
    // TODO: 初始化派生类特有成员
    self->conf = cfg;
	def_dev_init(self) = wdg_dev_init_impl;
    // 1. 检查是否由 IWDG 复位
    if (wdg_iwdg_reset_flag()) {
        // 发生了 IWDG 复位，记录或处理
        wdg_iwdg_clear();
    }

}

void wdg_deinit(Wdg* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void wdg_destroy(Wdg* self) {
    if (self != NULL) {
        wdg_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(wdg_dev_init_impl) {
    // TODO: add dev_init method
    Wdg *wdg = (Wdg *)self;
    //params 
    if (!wdg->conf || wdg->conf->reload > 0xFFF) {
        return;
    }

    // 1. 解锁 PR/RLR 寄存器 (写 0x5555 到 KR)
    xIWDG->KR = xIWDG_KEY_UNLOCK;

    // 2. 配置预分频器 (写 PR)
    xIWDG->PR = (uint32_t)(wdg->conf->prescaler & 0x07);

    // 3. 写重装载值 (写 RLR)
    xIWDG->RLR = (uint32_t)(wdg->conf->reload & 0x0FFF);

    // 4. 等待 SR 寄存器标志清除 (确保重装载完成)
    while (xIWDG->SR & (1 << 0));   // PVU (Prescaler Value Update)
    while (xIWDG->SR & (1 << 1));   // RVU (Reload Value Update)

    // 5. 启动看门狗 (写 0xCCCC 到 KR)
    xIWDG->KR = xIWDG_KEY_ENABLE;

    // 6. 立即喂狗一次 (写入 0xAAAA, 加载重装载值到计数器)
    xIWDG->KR = xIWDG_KEY_RELOAD;

}


// iwdg_reload method
static void wdg_iwdg_reload() {
    xIWDG->KR = xIWDG_KEY_RELOAD;
}
// iwdg_reset_flag method
static bool wdg_iwdg_reset_flag() {
    return (xRCC_CSR->CSR & xRCC_CSR_IWDGRSTF) != 0;
}
// iwdg_clear method
static void wdg_iwdg_clear() {
    xRCC_CSR->CSR |= xRCC_CSR_RMVF;
    
}
// iwdg_timeout method
static uint32_t wdg_iwdg_timeout(Wdg* self) {
    if (!self->conf) {
        return 0;
    }
    uint32_t prescaler_val = 4u << (self->conf->prescaler);   // 4,8,16,32,64,128,256,256
    uint32_t reload = self->conf->reload + 1;                   // 计数值为 reload+1 个周期
    // 周期 = prescaler_val / 32000 (s) = prescaler_val * 1000000 / 32000 (μs)
    // 简化：prescaler_val * 31.25 ≈ prescaler_val * 3125 / 100
    uint32_t timeout_us = (prescaler_val * 3125u / 100u) * reload;
    return timeout_us;
}


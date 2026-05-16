#include "fpu.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"
#include "main.h"
#include "../log/log.h"

dev_init_override(fpu_dev_init_impl);

// 析构函数声明
static void fpu_destroy(Fpu* self);
static bool fpu_irq_handler_impl(void *arg);

// TODO: 初始化数据成员
static const FpuFun fpu_fun = {
    .destroy = fpu_destroy,
};
// 构造函数实现
Fpu* fpu_create(const fpu_config_t *conf) {
    Fpu* obj = (Fpu*)os_malloc(sizeof(Fpu));
    if (obj) {
        memset(obj, 0, sizeof(Fpu));
        fpu_init(obj, conf);
    }
    return obj;
}

void fpu_init(Fpu* self, const fpu_config_t *conf) {
    // 初始化基类部分
    device_init(&self->base);
    self->fun = &(fpu_fun);
    // TODO: 初始化派生类特有成员

	def_dev_init(self) = fpu_dev_init_impl;
    self->conf = conf;
}

void fpu_deinit(Fpu* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void fpu_destroy(Fpu* self) {
    if (self != NULL) {
        fpu_deinit(self);
        os_free(self);
    }
}

// dev_init method
dev_init_override(fpu_dev_init_impl) {
    // TODO: add dev_init method
    Fpu *fpu = (Fpu *)self;
    //params
    LOG_DEBUG("fpu", "fpu init!");
    hal_fpu_init(fpu->conf);

    self->irq_conf.priority = self->fun->encode_pripority(self, 0, 0x00);
    self->irq_conf.handler = fpu_irq_handler_impl;
    self->irq_conf.semaphore = sem;
    self->irq_conf.arg = self;
    self->irq_conf.irq_num = UsageFault_IRQ;
    self->fun->attach_irq(self, &self->irq_conf);

}

static bool fpu_irq_handler_impl(void *arg) {
    uint32_t fpscr;
    __ASM volatile ("VMRS %0, FPSCR" : "=r"(fpscr));
    while (1); // 通常 FPU 异常会触发 HardFault 或 UsageFault
}


#include "rtc.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/linear_pool.h"

dev_ioctl_override(rtc_dev_ioctl_impl);

// 析构函数声明
static void rtc_destroy(Rtc* self);
static bool rtc_irq_handler_impl(nvic_irq_t *irq_conf);

// TODO: 初始化数据成员
static const RtcFun rtc_fun = {
    .destroy = rtc_destroy,
};
// 构造函数实现
Rtc* rtc_create(const device_info_t *info) {
    Rtc* obj = (Rtc*)os_malloc(sizeof(Rtc));
    if (obj) {
        memset(obj, 0, sizeof(Rtc));
        rtc_init(obj, info);
    }
    return obj;
}

void rtc_init(Rtc* self, const device_info_t *info) {
    // 初始化基类部分
    device_init(&self->base, info);
    self->fun = &(rtc_fun);
    // TODO: 初始化派生类特有成员
	def_dev_ioctl(self) = rtc_dev_ioctl_impl;
}

void rtc_deinit(Rtc* self) {
    device_deinit(GET_DEVICE(self));
    // TODO: 数据成员申请资源释放
}
// 析构函数实现
static void rtc_destroy(Rtc* self) {
    if (self != NULL) {
        rtc_deinit(self);
        os_free(self);
    }
}

// dev_ioctl method
dev_ioctl_override(rtc_dev_ioctl_impl) {
    // TODO: add dev_ioctl method
    Rtc *rtc = (Rtc *)self;
    const rtc_config_t *conf = self->info->conf;
    //params , int cmd, void *arg
    if (!conf) {
        return;
    }

    // 1. 使能备份域访问
    if (rtc_enable_backup_domain() != 0) return;

    // 2. 选择 RTC 时钟源
    if (rtc_select_clock_source(conf->clk_src) != 0) return;

    // 3. 解锁 RTC 寄存器
    rtc_unlock();

    // 4. 进入初始化模式
    if (rtc_enter_init_mode() != 0) return;

    // 5. 配置预分频器 (产生 1Hz ck_spre)
    xRTC->PRER = ((conf->async_prediv & 0x7F) << 16) |
                ((conf->sync_prediv & 0x7FFF) << 0);

    // 6. 配置小时格式
    hal_rtc_set_hour_format(conf->hour_format);

    // 7. 退出初始化模式
    rtc_exit_init_mode();

    // 8. 等待同步
    rtc_wait_sync();

    // 9. 中断配置
    if (conf->it_enable) {
        uint32_t cr = hal_rtc_read_cr();
        if (conf->it_enable & RTC_IT_ALARM_A)   cr |= xRTC_CR_ALRAIE;
        if (conf->it_enable & RTC_IT_ALARM_B)   cr |= xRTC_CR_ALRBIE;
        if (conf->it_enable & RTC_IT_WAKEUP)    cr |= xRTC_CR_WUTIE;
        if (conf->it_enable & RTC_IT_TIMESTAMP) cr |= xRTC_CR_TSIE;
        hal_rtc_write_cr(cr);

        self->irq_conf->handler = rtc_irq_handler_impl;
        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, RTC_ALARM_IRQ);
        self->fun->config_irq(self, self->irq_conf);

        self->irq_conf->irq_list->fun->add_int(self->irq_conf->irq_list, RTC_WKUP_IRQ);
        self->fun->config_irq(self, self->irq_conf);
    }

    rtc_lock();
}

static bool rtc_irq_handler_impl(nvic_irq_t *irq_conf) {
    uint32_t isr = hal_rtc_read_isr();
    uint32_t cr  = hal_rtc_read_cr();

    // 闹钟 A
    if ((isr & xRTC_ISR_ALRAF) && (cr & xRTC_CR_ALRAIE)) {
        hal_rtc_clear_isr_flag(xRTC_ISR_ALRAF);
    }
    // 闹钟 B
    if ((isr & xRTC_ISR_ALRBF) && (cr & xRTC_CR_ALRBIE)) {
        hal_rtc_clear_isr_flag(xRTC_ISR_ALRBF);
    }
    // 唤醒定时器
    if ((isr & xRTC_ISR_WUTF) && (cr & xRTC_CR_WUTIE)) {
        xRTC->ISR &= ~xRTC_ISR_WUTF;
        //if (rtc_callback) rtc_callback(RTC_EVT_WAKEUP);
    }
    // 时间戳
    if ((isr & xRTC_ISR_TSF) && (cr & xRTC_CR_TSIE)) {
        xRTC->ISR &= ~xRTC_ISR_TSF;
        //if (rtc_callback) rtc_callback(RTC_EVT_TIMESTAMP);
    }
    // 篡改
    if ((isr & xRTC_ISR_TAMP1F)) {
        xRTC->ISR &= ~xRTC_ISR_TAMP1F;
        //if (rtc_callback) rtc_callback(RTC_EVT_TAMPER);
    }
    return true;
}


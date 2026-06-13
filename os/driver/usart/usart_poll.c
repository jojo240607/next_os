/**
 * USART 轮询模式
 */
#include "usart.h"
#include "../hal/hal_usart.h"

void usart_poll_dev_init(Device *self) { (void)self; }
void usart_poll_ioctl(Device *self, ioctl_cmd_t cmd, void *arg) { (void)self; (void)cmd; (void)arg; }

size_t usart_poll_read(Device *self, void *buf, size_t count) {
    const usart_config_t *conf = self->info->conf;
    if (conf->id >= UART_MAX || count == 0) return 0;
    return hal_uart_recv(conf->id, (uint8_t *)buf, count);
}

void usart_poll_write(Device *self, const void *buf, size_t count) {
    const usart_config_t *conf = self->info->conf;
    if (conf->id >= UART_MAX || count == 0) return;
    hal_uart_send(conf->id, (uint8_t *)buf, (uint16_t)count);
}

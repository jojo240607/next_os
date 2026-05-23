//
// Created by zhiwei.gong on 2026/5/12.
//

#include "hal_i2c.h"
#include "../common/rcc.h"
#include "../common/dma.h"

/* ---------- I2C 寄存器定义 (STM32F4) ---------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
    volatile uint32_t FLTR;
} xI2C_TypeDef;

static xI2C_TypeDef* const I2Cx[] = {
        (xI2C_TypeDef*)xI2C1_BASE,
        (xI2C_TypeDef*)xI2C2_BASE,
        (xI2C_TypeDef*)xI2C3_BASE
};

/* ---------- 时钟与频率配置 ---------- */
void hal_i2c_clock_enable(i2c_id_t id)
{
    switch (id) {
        case I2C_1:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2C1EN);
            break;
        case I2C_2:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2C2EN);
            break;
        case I2C_3:
            rcc_periph_clock_enable(RCC_BUS_APB1, xRCC_APB1ENR_I2C3EN);
            break;
        default:
            return;
    }
    __asm volatile ("dsb" ::: "memory");
}

void hal_i2c_set_clock(i2c_id_t id, uint32_t target_speed)
{
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    //uint32_t pclk = hal_rcc_get_apb1_clock();//42000000;  /* APB1 时钟 42MHz */
    uint32_t pclk = hal_rcc_get_apb1_clock();
    /* 配置 CR2 的 FREQ 字段 (单位 MHz) */
    uint32_t freq = pclk / 1000000;
    i2c_ctrl->CR2 = (i2c_ctrl->CR2 & ~0x3F) | (freq & 0x3F);

    /* 计算 CCR */
    uint32_t ccr;
    if (target_speed <= 100000) {
        // 标准模式
        ccr = pclk / (target_speed * 2);
        if (ccr < 4) ccr = 4;
        i2c_ctrl->CCR = ccr & 0xFFF;
    } else {
        // 快速模式 (占空比 16:9)
        ccr = pclk / (target_speed * 25);
        if (ccr < 1) ccr = 1;
        i2c_ctrl->CCR = (1 << 15) | (ccr & 0xFFF);
    }

    /* 配置 TRISE */
    if (target_speed <= 100000) {
        i2c_ctrl->TRISE = freq + 1;
    } else {
        i2c_ctrl->TRISE = (freq * 300) / 1000 + 1;
    }
}

void hal_i2c_reset(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    i2c_ctrl->CR1 = (1 << 15);  /* SWRST */
    i2c_ctrl->CR1 = 0;
}

void hal_i2c_set_addr(i2c_id_t id, i2c_addr_mode_t addr_mode, uint8_t own_address) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    if (addr_mode == I2C_ADDR_7BIT) {
        i2c_ctrl->OAR1 = (own_address & 0x7F) << 1;
        i2c_ctrl->OAR1 &= ~(1 << 15);  /* 7 位模式 */
    } else {
        i2c_ctrl->OAR1 = (own_address & 0x3FF) | (1 << 15);
    }
}

void hal_i2c_enable(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    i2c_ctrl->CR1 |= I2C_CTL_ACK | I2C_CTL_PE;
}

bool hal_i2c_it_init(i2c_id_t id, i2c_it_t it_enable) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    uint32_t cr2 = 0;
    if (it_enable) {
        if (it_enable & (xI2C_IT_TXE | xI2C_IT_RXNE)) {
            cr2 |= xI2C_IE_ITBUFEN | xI2C_IE_ITEVTEN;

        }
        if (it_enable & xI2C_IT_ERR) {
            cr2 |= xI2C_IE_ITERREN;
        }
        i2c_ctrl->CR2 = cr2;
        return true;
    }
    return false;
}

void hal_i2c_dma_init(i2c_id_t id, bool txdma, bool rxdma) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    if (txdma) {
        i2c_ctrl->CR2 |= xI2C_IE_TXDMAEN;   // 使能发送 DMA (TXDMAEN)
    }
    if (rxdma) {
        i2c_ctrl->CR2 |= xI2C_IE_RXDMAEN;   // 使能接收 DMA (RXDMAEN)
    }
}

uint32_t hal_i2c_get_it_event(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    return i2c_ctrl->SR1;
}

volatile uint8_t *hal_i2c_addr(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    return (volatile uint8_t *)&i2c_ctrl->DR;
}


// transmit method
void i2c_transmit(i2c_id_t id, uint8_t slave_addr, const uint8_t *data, uint16_t len) {
    // TODO: add transmit method
    if (id >= I2C_MAX) {
        return;
    }
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];

    /* 发送起始条件 */
    i2c_ctrl->CR1 |= I2C_CTL_START;
    while (!(i2c_ctrl->SR1 & I2C_FLG_SB));
    /* 发送地址 (写) */
    i2c_ctrl->DR = (slave_addr << 1) | 0x00;   /* LSB=0 表示写 */
    while (!(i2c_ctrl->SR1 & I2C_FLG_ADDR));
    (void)i2c_ctrl->SR2;   /* 读 SR2 清除 ADDR 标志 */

    /* 发送数据 */
    for (int i = 0; i < len; i++) {
        while (!(i2c_ctrl->SR1 & I2C_FLG_TXE));
        i2c_ctrl->DR = data[i];
    }
    /* 等待传输完成 */
    while (!(i2c_ctrl->SR1 & I2C_FLG_BTF));
    /* 发送停止条件 */
    i2c_ctrl->CR1 |= I2C_CTL_STOP;
}


// receive method
void i2c_receive(i2c_id_t id, uint8_t slave_addr, uint8_t *buffer, uint16_t len) {
    // TODO: add receive method
    if (id >= I2C_MAX) {
        return;
    }
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];

    /* 发送起始条件 */
    i2c_ctrl->CR1 |= I2C_CTL_START;
    while (!(i2c_ctrl->SR1 & I2C_FLG_SB));
    /* 发送地址 (读) */
    i2c_ctrl->DR = (slave_addr << 1) | 0x01;   /* LSB=1 表示读 */
    while (!(i2c_ctrl->SR1 & I2C_FLG_ADDR));
    (void)i2c_ctrl->SR2;

    /* 接收数据 */
    for (int i = 0; i < len; i++) {
        if (i == len - 1) {
            /* 最后一个字节前关闭 ACK */
            i2c_ctrl->CR1 &= ~I2C_CTL_ACK;
        }
        while (!(i2c_ctrl->SR1 & I2C_FLG_RXNE));
        buffer[i] = i2c_ctrl->DR;
    }
    /* 发送停止条件 */
    i2c_ctrl->CR1 |= I2C_CTL_STOP;

    /* 恢复 ACK */
    i2c_ctrl->CR1 |= I2C_CTL_ACK;
}


void hal_i2c_transmit_it_start(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    /* 使能中断（若未打开） */
    i2c_ctrl->CR2 |= xI2C_IE_ITBUFEN | xI2C_IE_ITEVTEN;
    /* 发送起始条件 */
    i2c_ctrl->CR1 |= I2C_CTL_START;
}

void hal_i2c_receive_it_start(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    i2c_ctrl->CR2 |= xI2C_IE_ITBUFEN | xI2C_IE_ITEVTEN;
    i2c_ctrl->CR1 |= I2C_CTL_START;
}

void hal_i2c_clear_addr_flag(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    uint16_t sr2 = i2c_ctrl->SR2;
}

void hal_i2c_close_ack(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    i2c_ctrl->CR1 &= ~I2C_CTL_ACK;
}

void hal_i2c_stop(i2c_id_t id) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    i2c_ctrl->CR1 |= I2C_CTL_STOP;
}

void hal_i2c_clear_it_event(i2c_id_t id, i2c_it_t it_event) {
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    i2c_ctrl->CR2 &= ~it_event;  // 关闭 TXE 中断
}
// transmit_dma method
void i2c_transmit_dma(i2c_id_t id, const dma_stream_config_t *cfg, uint8_t slave_addr, const uint8_t *data, uint16_t len) {
    // TODO: add transmit_dma method
    if (id >= I2C_MAX) {
        return;
    }
    xI2C_TypeDef *i2c_ctrl = I2Cx[id];
    // 初始化 DMA 传输
    dma_start_transfer(cfg,
                       (uint32_t)data, (uint32_t)&i2c_ctrl->DR, len);
    // 发送起始条件，地址等由软件完成第一个字节，之后 DMA 接管
    i2c_ctrl->CR1 |= I2C_CTL_START;
    while (!(i2c_ctrl->SR1 & I2C_FLG_SB));
    i2c_ctrl->DR = (slave_addr << 1) | 0x00;  // 写地址
    while (!(i2c_ctrl->SR1 & I2C_FLG_ADDR));  // 等待地址发送完毕
    (void)i2c_ctrl->SR2;
}

//
// 驱动初始化配置参数表
//
#include "device_config.h"
/*
 *
 * 简单模式：使用默认 HSI (16MHz)
rcc_sysclk_config_t clk_cfg = {
    .sysclk_src = RCC_CLK_HSI,
    .hsi_freq   = 16000000,
    // 其他字段不重要
};
 */
//168MHz (HSE 8MHz + PLL 168M)
const rcc_sysclk_config_t clk_conf = {
        .sysclk_src  = RCC_CLK_PLL,
        .pll_src     = RCC_PLLSRC_HSE,
        .hse_bypass  = false,         // 无源晶振
        .hse_freq    = 8000000,       //8M
        .hsi_freq    = 16000000,      //16M

        .pll_m = 8,                   // 8MHz / 8 = 1MHz
        .pll_n = 336,                 // 1MHz * 336 = 336MHz (VCO)
        .pll_p = 2,                   // 336MHz / 2 = 168MHz (系统时钟)
        .pll_q = 7,                   // 336MHz / 7 = 48MHz (USB/ENET)
        .ahb_div  = RCC_AHB_DIV1,     // HCLK = 168MHz
        .apb1_div = RCC_APB_DIV4,     // APB1 = 42MHz (最大42)
        .apb2_div = RCC_APB_DIV2,     // APB2 = 84MHz
        .target_sysclk = 168000000
};
const systick_config_t sys_tick_conf = {
        .interval_us  = 1000,        // 1ms
        .one_shot     = false,       // 周期性
};
/*
 *      APB 预分频 = 1 → 定时器时钟 = APB 时钟
        APB 预分频 ≠ 1 → 定时器时钟 = 2 × APB 时钟
        → 因此定时器时钟 = 84 MHz。
        所有挂载在 APB1 和 APB2 上的定时器都适用此规则，包括：
        APB1：TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14
        APB2：TIM1, TIM8, TIM9, TIM10, TIM11
 */
const tim_config_t time2_conf = {
        .id = TIM_2,
        .timebase = {
                .counter_mode = TIM_COUNTER_UP,
                .prescaler    = 41999,        // 84MHz / 42000 = 2000 Hz
                .autoreload   = 1999,         // 2000 / 2000 = 1 Hz (0.1秒中断)
                .clock_division = 0,
                .repetition  = 0
        },
        .it_enable = TIM_IT_UPDATE,
};

const usart_config_t usart1_conf = {
        .id = UART_1,
        .baudrate = 115200,
        .word_len = UART_WORDLEN_8,
        .stop_bits = UART_STOP_1,
        .parity = UART_PARITY_NONE,
        .pins = {
                .uart_tx = PA9_REQ_USART1_TX,
                .uart_rx = PA10_REQ_USART1_RX,
        },
        .it_enable = xUART_IT_TXE | xUART_IT_RXNE | xUART_IT_TC | xUART_FLAG_IDLE,
        .cache_size = 64,
        .dma_cfg = &(const uart_dma_config_t) {
                .tx_dma = &(const dma_stream_config_t) {
                        .dma_request    = DMA2_REQ_USART1_TX,
                        .direction      = DMA_DIR_M2P,
                        .priority       = xDMA_PRIORITY_HIGH,
                        .mem_data_size  = DMA_DATA_SIZE_BYTE,
                        .per_data_size  = DMA_DATA_SIZE_BYTE,
                        .mem_inc        = 1,
                        .per_inc        = 0,
                        .mode           = DMA_MODE_NORMAL,     // 单次发送
                        .fifo_mode      = DMA_FIFO_DIRECT,
                        .it_enable      = xDMA_IT_TC,           // 只使能传输完成中断
                },
                .rx_dma = &(const dma_stream_config_t) {
                        .dma_request    = DMA2_REQ_USART1_RX_ST2,
                        .direction      = DMA_DIR_P2M,
                        .priority       = xDMA_PRIORITY_HIGH,
                        .mem_data_size  = DMA_DATA_SIZE_BYTE,
                        .per_data_size  = DMA_DATA_SIZE_BYTE,
                        .mem_inc        = 1,
                        .per_inc        = 0,
                        .mode           = DMA_MODE_CIRCULAR, // 接收推荐循环模式
                        .fifo_mode      = DMA_FIFO_DIRECT,
                        .it_enable      = xDMA_IT_HT | xDMA_IT_TC,                 // 双缓冲接收dma数据，更快速处理接收数据，且不会遗漏
                },
        },
};

const usart_config_t usart4_conf = {
        .id = UART_4,
        .baudrate = 115200,
        .word_len = UART_WORDLEN_8,
        .stop_bits = UART_STOP_1,
        .parity = UART_PARITY_NONE,
        .pins = {
                .uart_tx = PC10_REQ_UART4_TX,
                .uart_rx = PC11_REQ_UART4_RX,
        },
        .it_enable = xUART_IT_TXE | xUART_IT_RXNE | xUART_IT_TC,
        .cache_size = 64,
        .dma_cfg = &(const uart_dma_config_t) {
                .tx_dma = &(const dma_stream_config_t) {
                    .dma_request    = DMA1_REQ_UART4_TX,
                    .direction      = DMA_DIR_M2P,
                    .priority       = xDMA_PRIORITY_HIGH,
                    .mem_data_size  = DMA_DATA_SIZE_BYTE,
                    .per_data_size  = DMA_DATA_SIZE_BYTE,
                    .mem_inc        = 1,
                    .per_inc        = 0,
                    .mode           = DMA_MODE_NORMAL,     // 单次发送
                    .fifo_mode      = DMA_FIFO_DIRECT,
                    .it_enable      = xDMA_IT_TC,           // 只使能传输完成中断
                },
                .rx_dma = &(const dma_stream_config_t) {
                    .dma_request    = DMA1_REQ_UART4_RX,
                    .direction      = DMA_DIR_P2M,
                    .priority       = xDMA_PRIORITY_HIGH,
                    .mem_data_size  = DMA_DATA_SIZE_BYTE,
                    .per_data_size  = DMA_DATA_SIZE_BYTE,
                    .mem_inc        = 1,
                    .per_inc        = 0,
                    .mode           = DMA_MODE_CIRCULAR, // 接收推荐循环模式
                    .fifo_mode      = DMA_FIFO_DIRECT,
                    .it_enable      = 0,                 // 直接读取缓冲区则无需中断
                },
        },
};

const exti_config_t exti_conf = {
        .pin_size = 2,
        .pin_conf = {&(const exti_pin_cfg_t) {
                        .port = PORT_A,
                        .pin = 1,
                        .exti_mode = PIN_IRQ_MODE_FALLING,
                     },
                     &(const exti_pin_cfg_t) {
                        .port = PORT_B,
                        .pin = 5,
                        .exti_mode = PIN_IRQ_MODE_FALLING,
                     }
        }
};

const adc_config_t adc1_conf = {
        .id          = ADC_1,
        .resolution  = ADC_RES_12BIT,
        .align       = ADC_ALIGN_RIGHT,
        .continuous  = true,            // 单次, 连续转换模式 (若不使用 DMA 请慎用)
        .dma_cfg = NULL/*&(const dma_stream_config_t) {
                .dma_request    = DMA2_REQ_ADC1_ST0,
                .direction      = DMA_DIR_P2M,        // 外设到存储器
                .priority       = xDMA_PRIORITY_HIGH,
                .mem_data_size  = DMA_DATA_SIZE_HALFWORD, // 16 位半字
                .per_data_size  = DMA_DATA_SIZE_HALFWORD,
                .mem_inc        = 1,
                .per_inc        = 0,
                .mode           = DMA_MODE_CIRCULAR,   // 循环扫描
                .fifo_mode      = DMA_FIFO_DIRECT,
                .it_enable      = xDMA_IT_TC,          // 可选，一轮完成中断
            }*/,
        .num_channels = 2,
        .channels    = {
                &(const adc_channel_cfg_t) {
                    .channel = 0,
                    .sample_time = ADC_SMP_480CYCLES,
                    .port = PORT_A,
                    .pin = PIN_0,},
                &(const adc_channel_cfg_t){
                    .channel = 1,
                    .sample_time = ADC_SMP_480CYCLES,
                    .port = PORT_A,
                    .pin = PIN_2,}
        },
};


const spi_config_t spi1_conf = {
        .id = SPI_1,
        .mode = SPI_MODE_3,            // ICM20948: CPOL=1, CPHA=1
        .frame_format = SPI_FRAME_8BIT,
        .baudrate_div = SPI_BR_DIV64,   // 84MHz/128 ≈ 656kHz
        .master = SPI_MASTER,
        .first_bit = SPI_MSB_FIRST,    // MSB 优先
        .nss_mode  = SPI_NSS_SOFT,     // 软件 NSS → Renode 调试更可靠
        .cache_size = 64,              // ringbuf 大小
        .pins = {
                .sck_pin  = PA5_REQ_SPI1_SCK,
                .miso_pin = PA6_REQ_SPI1_MISO,
                .mosi_pin = PA7_REQ_SPI1_MOSI,
                .nss_pin  = PA4_REQ_SPI1_NSS,   // 硬件 NSS 或软件管理
                .cs_pin = {.port = PORT_A, .pin = PIN_4},//软件 cs
        },
        .it_enable = xSPI_IT_TXE | xSPI_IT_RXNE,//xSPI_IT_NONE,
        .dma_cfg = &(const spi_dma_config_t){
                .tx_dma = &(const dma_stream_config_t) {
                        .dma_request = DMA2_REQ_SPI1_TX_ST3,
                        .direction = DMA_DIR_M2P,
                        .priority = xDMA_PRIORITY_HIGH,
                        .mem_data_size = DMA_DATA_SIZE_BYTE,
                        .per_data_size = DMA_DATA_SIZE_BYTE,
                        .mem_inc = 1,
                        .per_inc = 0,
                        .mode = DMA_MODE_NORMAL,
                        .fifo_mode = DMA_FIFO_DIRECT,
                        .it_enable = xDMA_IT_TC,
                },
                .rx_dma = &(const dma_stream_config_t) {
                        .dma_request = DMA2_REQ_SPI1_RX_ST2,
                        .direction = DMA_DIR_P2M,
                        .priority = xDMA_PRIORITY_HIGH,
                        .mem_data_size = DMA_DATA_SIZE_BYTE,
                        .per_data_size = DMA_DATA_SIZE_BYTE,
                        .mem_inc = 1,
                        .per_inc = 0,
                        .mode = DMA_MODE_NORMAL,
                        .fifo_mode = DMA_FIFO_DIRECT,
                        .it_enable = xDMA_IT_NONE,  // 只靠 TX TC 通知完成
                },
        },
};

const i2c_config_t i2c1_conf = {
        .id = I2C_1,
        .clock_speed = 100000,          // 100 kHz 标准模式
        .addr_mode = I2C_ADDR_7BIT,
        .own_address = 0x00,            // 主模式不关心
        .pins = {
                .scl_pin = PB6_REQ_I2C1_SCL,
                .sda_pin = PB7_REQ_I2C1_SDA
        },
        .it_enable = xI2C_IT_TXE | xI2C_IT_RXNE,                 // 不使用中断
        .dma_cfg = NULL/*&(const i2c_dma_config_t){
                .tx_dma = &(const dma_stream_config_t) {
                        .dma_request = DMA1_REQ_I2C1_TX_ST6,
                        .direction = DMA_DIR_M2P,
                        .priority = xDMA_PRIORITY_HIGH,
                        .mem_data_size = DMA_DATA_SIZE_BYTE,
                        .per_data_size = DMA_DATA_SIZE_BYTE,
                        .mem_inc = 1,
                        .per_inc = 0,
                        .mode = DMA_MODE_NORMAL,
                        .fifo_mode = DMA_FIFO_DIRECT,
                        .it_enable = xDMA_IT_TC,
                },
                .rx_dma = &(const dma_stream_config_t) {
                        .dma_request = DMA1_REQ_I2C1_RX_ST0,
                        .direction = DMA_DIR_P2M,
                        .priority = xDMA_PRIORITY_HIGH,
                        .mem_data_size = DMA_DATA_SIZE_BYTE,
                        .per_data_size = DMA_DATA_SIZE_BYTE,
                        .mem_inc = 1,
                        .per_inc = 0,
                        .mode = DMA_MODE_NORMAL,
                        .fifo_mode = DMA_FIFO_DIRECT,
                        .it_enable = xDMA_IT_NONE,  // 只靠 TX TC 通知完成
                },
        },*/
};

const can_config_t can1_conf = {
        .id = CAN_1,
        .mode = CAN_MODE_NORMAL,
        .prescaler = 7,      // 42MHz/(7+1)=5.25MHz 量化时钟
        .sjw = 1,
        .bs1 = 8,            // 11 time quanta
        .bs2 = 3,
        .auto_bus_off = true,
        .auto_wakeup = false,
        .no_auto_retrans = false,
        .pins = { .can_tx = PD1_REQ_CAN1_TX,
                  .can_rx = PD0_REQ_CAN1_RX},
        .it_enable = CAN_IT_FMP0 | CAN_IT_FMP1,
        .num_filters = 2,
        .filters =  {
                &(can_filter_config_t){
                        .bank = 0,     // 滤波器0: 匹配高优先级ID -> 存FIFO0
                        .mode = CAN_FILTER_LIST_MODE,
                        .scale = CAN_FILTER_32BIT,
                        .fifo = CAN_FIFO0,                   // 关键：高优先级报文进FIFO0
                        .id_high = (0x100 << 21),   // 替换为实际CAN ID 0x100
                        .active = true
                },
                &(can_filter_config_t) {
                        .bank = 1,     // 滤波器1: 匹配其他ID -> 存FIFO1
                        .mode = CAN_FILTER_MASK_MODE,
                        .scale = CAN_FILTER_32BIT,
                        .fifo = CAN_FIFO1,                   // 低优先级报文进FIFO1
                        .id_high = 0x000,
                        .active = true
                }
            },

};

const pwm_config_t pwm1_conf = {
        .timer_id = TIM_1,
        .timebase = {
                .counter_mode = TIM_COUNTER_UP,
                .prescaler    = 83,
                .autoreload   = 999,
                .clock_division = 0,
                .repetition  = 0
        },
        .num_channels = 3,
        .channels = {
                &(const pwm_channel_t) { .channel=1, .mode=TIM_OC_MODE_PWM1, .duty=300, .enable_preload=true, .pwm_pin = PA8_REQ_TIM1_CH1},
                &(const pwm_channel_t) { .channel=2, .mode=TIM_OC_MODE_PWM2, .duty=500, .enable_preload=true, .pwm_pin = PE11_REQ_TIM1_CH2},
                &(const pwm_channel_t) { .channel=3, .mode=TIM_OC_MODE_PWM1, .duty=800, .enable_preload=true, .pwm_pin = PE13_REQ_TIM1_CH3}
        }
};

const i2s_config_t i2s2_conf = {
        .id             = I2S_2,
        .mode           = xI2S_MODE_MASTER_TX,
        .standard       = xI2S_STANDARD_PHILIPS,
        .data_format    = I2S_DATA_16BIT,
        .clock_polarity = I2S_CKPOL_LOW,
        .audio_freq     = 44100,
        .plli2s_n       = 258,          // PLLI2SN (258MHz VCO)
        .plli2s_r       = 3,            // PLLI2SR (86MHz I2S时钟)
        .i2s_div        = 7,            // I2SDIV (分频产生WS)
        .odd_factor     = false,
        .enable_mck     = false,
        .pins = {
                .sck_pin = PB13_REQ_I2S2_CK,
                .ws_pin  = PB12_REQ_I2S2_WS,
                .sd_pin  = PC3_REQ_I2S2_SD,
                .mck_pin = PC6_REQ_I2S2_MCK
        },
        .it_enable  = xI2S_IT_NONE,
        .dma_cfg = NULL
};

const fsmc_config_t fsmc_conf = {
        .pins = {
                .data0_pin = PD14_REQ_FSMC_D0,
                .data1_pin = PD15_REQ_FSMC_D1,
                .data2_pin = PD0_REQ_FSMC_D2,
                .data3_pin = PD1_REQ_FSMC_D3,
                .data4_pin = PE7_REQ_FSMC_D4,
                .data5_pin = PE8_REQ_FSMC_D5,
                .data6_pin = PE9_REQ_FSMC_D6,
                .data7_pin = PE10_REQ_FSMC_D7,
                .data8_pin = PE11_REQ_FSMC_D8,
                .data9_pin = PE12_REQ_FSMC_D9,
                .data10_pin = PE13_REQ_FSMC_D10,
                .data11_pin = PE14_REQ_FSMC_D11,
                .data12_pin = PE15_REQ_FSMC_D12,
                .data13_pin = PD8_REQ_FSMC_D13,
                .data14_pin = PD9_REQ_FSMC_D14,
                .data15_pin = PD10_REQ_FSMC_D15,
                .ne_pin = PD7_REQ_FSMC_NE1,
                .a_pin = PD11_REQ_FSMC_A16,
                .rd_pin = PD4_REQ_FSMC_NOE,
                .wr_pin = PD5_REQ_FSMC_NWE,
        },
        .address_setup_time  = 0x0F,
        .data_setup_time     = 60,
        .bus_turnaround_time = 0,
        .dma_cfg = &(dma_stream_config_t) {
                .dma_request = DMA2_REQ_TIM1_CH3_ST6_CN0,
                .direction = DMA_DIR_M2M,
                .priority = xDMA_PRIORITY_HIGH,
                .mem_data_size = DMA_DATA_SIZE_HALFWORD,
                .per_data_size = DMA_DATA_SIZE_HALFWORD,
                .mem_inc = 1,
                .per_inc = 0,
                .mode = DMA_MODE_NORMAL,
                .fifo_mode = DMA_FIFO_DIRECT,
                .it_enable = xDMA_IT_TC,
        },
};

const lcd_fsmc_config_t lcd_fsmc_conf = {
        .bus_dev_id   = DEVICE_FSMC,
        .width        = 320,
        .height       = 240,
        .rs_addr_line = 16,
        .rst_pin      = {.port = PORT_D, .pin = PIN_13},
        .bl_pin       = {.port = PORT_D, .pin = PIN_12},
        .init_sequence = NULL,
};
const iwdg_config_t iwdg_conf = {
        .prescaler = IWDG_PRESCALER_64,
        .reload    = 1250       // (0..4095)
};

/* ── 片外设备驱动配置 ── */
const icm20948_config_t icm20948_conf = {
        .spi_id = SPI_1,
        .cs     = {.port = PORT_A, .pin = PIN_4},   /* ICM20948 CS = PA4 */
};

const adxl345_config_t adxl345_conf = {
        .i2c_id      = I2C_1,
        .slave_addr  = 0x53,                         /* ADXL345 I2C 地址 */
};

const usb_cdc_config_t usb_cdc_conf = {
        .pins = {
                .usb_dm = PA11_REQ_OTG_FS_DM,
                .usb_dp = PA12_REQ_OTG_FS_DP,
        },
        .manufacturer_str = "STM32F4",
        .product_str      = "USB CDC Test",
        .serial_str       = "0001",
        /* device_desc / config_desc 留 NULL 使用内置默认值 */
};

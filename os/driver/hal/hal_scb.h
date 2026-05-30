//
// Created by zhiwei.gong on 2026/5/19.
//

#ifndef STM32F4DISCOVERY_HAL_SCB_H
#define STM32F4DISCOVERY_HAL_SCB_H
#include <stdint.h>
#include <stdbool.h>

/* SCB 寄存器定义 (与 scb.h 中相同，这里直接引用) */
#define xSCB_BASE  0xE000ED00UL
#define xSCB       ((xSCB_TypeDef *)xSCB_BASE)

typedef struct {
    volatile const uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint8_t  SHP[12];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;   // 可配置故障状态
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
    volatile uint32_t AFSR;
} xSCB_TypeDef;

/* AIRCR 位 */
#define xSCB_AIRCR_VECTKEY       (0x5FAUL << 16)
#define xSCB_AIRCR_PRIGROUP_Pos  8
#define xSCB_AIRCR_PRIGROUP_Msk  (0x07UL << xSCB_AIRCR_PRIGROUP_Pos)
#define xSCB_AIRCR_SYSRESETREQ   (1UL << 2)

/* SCR 位 */
#define xSCB_SCR_SLEEPONEXIT    (1UL << 1)
#define xSCB_SCR_SLEEPDEEP      (1UL << 2)
#define xSCB_SCR_SEVONPEND      (1UL << 4)

/* CCR 位 */
#define xSCB_CCR_DIV_0_TRP      (1UL << 4)
#define xSCB_CCR_UNALIGN_TRP    (1UL << 3)

/* ICSR 位 */
#define xSCB_ICSR_VECTACTIVE_Pos 0

#define xSCB_SHCSR_MEMFAULTENA_Msk (1UL << 16) // MemManage (内存管理) 使能位
#define xSCB_SHCSR_BUSFAULTENA_Msk (1UL << 17) // BusFault (总线错误) 使能位
#define xSCB_SHCSR_USGFAULTENA_Msk (1UL << 18) // UsageFault (用法错误) 使能位

/* 优先级分组 (复用 NVIC 定义) */
/* NVIC 优先级分组 */
typedef enum : uint8_t {
    SCB_PRIORITY_GROUP_0 = 0x07,    // 0位抢占, 4位子优先级
    SCB_PRIORITY_GROUP_1 = 0x06,    // 1位抢占, 3位子优先级
    SCB_PRIORITY_GROUP_2 = 0x05,    // 2位抢占, 2位子优先级
    SCB_PRIORITY_GROUP_3 = 0x04,    // 3位抢占, 1位子优先级
    SCB_PRIORITY_GROUP_4 = 0x03     // 4位抢占, 0位子优先级
} scb_priority_group_t;

/* 睡眠模式 */
typedef enum : uint8_t {
    SCB_SLEEP_EXIT = 0,          // 立即睡眠
    SCB_SLEEP_ON_EXIT = 1        // 退出异常后睡眠
} scb_sleep_mode_t;

/* SCB 配置描述符 */
typedef struct {
    scb_priority_group_t priority_group;    // 优先级分组
    uint32_t             vector_table_base; // 向量表基地址 (如 0x08000000)
    bool                 enable_fault_trap; // 使能故障捕获 (除零、非对齐访问等)
    bool                 enable_sleep_on_exit; // 使能“退出异常时睡眠”
    bool                 enable_deep_sleep;    // 使能深度睡眠 (配合 WFI)
} scb_config_t;

/* ========== API ========== */
int  hal_scb_init(const scb_config_t *cfg);
void hal_scb_deinit(void);

void hal_scb_system_reset(void);
void hal_scb_set_vector_table(uint32_t base);
void hal_scb_set_priority_group(scb_priority_group_t group);

/* 故障状态查询与清除 */
uint32_t hal_scb_get_fault_status(void);
void     hal_scb_clear_fault_status(void);

/* 睡眠模式配置 */
void hal_scb_set_sleep_mode(scb_sleep_mode_t mode);
void hal_scb_enable_deep_sleep(bool enable);

/* CPU ID 读取 */
uint32_t hal_scb_get_cpuid(void);

/* 中断控制与状态 */
uint32_t hal_scb_get_vect_active(void);
bool     hal_scb_is_in_exception(void);
#endif //STM32F4DISCOVERY_HAL_SCB_H

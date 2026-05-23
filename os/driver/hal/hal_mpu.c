//
// Created by zhiwei.gong on 2026/5/21.
//

#include "hal_mpu.h"
#include "hal_scb.h"            /* 使用 SCB 结构体访问 MPU 寄存器 */
#include <stddef.h>
#include "cmsis_gcc.h"
#include "hal_fault_diag.h"
#include "../../log/log.h"

/* ── MPU 寄存器基地址 (Cortex-M4) ── */
#define xMPU_BASE    0xE000ED90UL
typedef struct {
    volatile uint32_t TYPE;     /* 0x00  MPU 类型寄存器 (只读) */
    volatile uint32_t CTRL;     /* 0x04  控制寄存器 */
    volatile uint32_t RNR;      /* 0x08  区域编号寄存器 */
    volatile uint32_t RBAR;     /* 0x0C  区域基地址寄存器 */
    volatile uint32_t RASR;     /* 0x10  区域属性与大小寄存器 */
} xMPU_TypeDef;
#define xMPU  ((xMPU_TypeDef *)xMPU_BASE)

/* ── CTRL 位 ── */
#define xMPU_CTRL_ENABLE         (1 << 0)
#define xMPU_CTRL_HFNMIENA       (1 << 1)
#define xMPU_CTRL_PRIVDEFENA     (1 << 2)

/* ── RBAR 位 ── */
#define xMPU_RBAR_VALID          (1 << 4)
#define xMPU_RBAR_REGION_Pos     0
#define xMPU_RBAR_ADDR_Pos       5

/* ── RASR 位 ── */
#define xMPU_RASR_ENABLE         (1 << 0)
#define xMPU_RASR_SIZE_Pos       1
#define xMPU_RASR_SIZE_Msk       (0x1F << xMPU_RASR_SIZE_Pos)
#define xMPU_RASR_ATTR_Pos       16
#define xMPU_RASR_ATTR_Msk       (0xFFFF << xMPU_RASR_ATTR_Pos)
#define xMPU_RASR_AP_Pos         24
#define xMPU_RASR_AP_Msk         (0x07 << xMPU_RASR_AP_Pos)
#define xMPU_RASR_TEX_Pos        19
#define xMPU_RASR_TEX_Msk        (0x07 << xMPU_RASR_TEX_Pos)
#define xMPU_RASR_S_Pos          18
#define xMPU_RASR_C_Pos          17
#define xMPU_RASR_B_Pos          16
#define xMPU_RASR_XN_Pos         28
#define xMPU_RASR_SRD_Pos        8
#define xMPU_RASR_SRD_Msk        (0xFF << xMPU_RASR_SRD_Pos)

/* ── 内部状态 ── */


/* ===================================================================
   初始化
   =================================================================== */
int mpu_init(const mpu_config_t *cfg)
{
    if (!cfg) {
        return -1;
    }
    LOG_DEBUG("mpu", "mpu init");
    /* 1. 禁用 MPU 以确保配置期间安全 */
    xMPU->CTRL &= ~xMPU_CTRL_ENABLE;
    __DSB();
    __ISB();

    /* 2. 配置背景区域 */
    uint32_t ctrl = 0;
    if (cfg->enable_default_map) {
        ctrl |= xMPU_CTRL_PRIVDEFENA;
    }

    /* 3. 逐一配置各个区域 */
    if (cfg->regions && cfg->num_regions > 0) {
        for (uint8_t i = 0; i < cfg->num_regions; i++) {
            if (hal_mpu_region_set(cfg->regions[i]) != 0) {
                return -1;
            }
        }
    }

    /* 4. 使能 MPU 并保持配置 */
    xMPU->CTRL = ctrl | xMPU_CTRL_ENABLE;
    __DSB();
    __ISB();

    //mpu_initialized = true;
    return 0;
}

void mpu_deinit(void)
{
    xMPU->CTRL = 0;               /* 禁用 MPU */
    __DSB();
    __ISB();
}

/* ===================================================================
   使能 / 禁止
   =================================================================== */
void mpu_enable(void)
{
    xMPU->CTRL |= xMPU_CTRL_ENABLE;
    __DSB();
    __ISB();
}

void mpu_disable(void)
{
    xMPU->CTRL &= ~xMPU_CTRL_ENABLE;
    __DSB();
    __ISB();
}

/* ===================================================================
   配置/修改一个区域
   =================================================================== */
int hal_mpu_region_set(const mpu_region_config_t *region)
{
    if (!region || region->region_num >= MPU_MAX_REGIONS) {
        return -1;
    }

    /* 1. 选择区域编号 */
    xMPU->RNR = region->region_num;

    /* 2. 写入基地址 (与 VALID 位组合) */
    uint32_t rbar = (region->base_address & 0xFFFFFFE0) | xMPU_RBAR_VALID |
                    (region->region_num & 0xF);
    xMPU->RBAR = rbar;

    /* 3. 构造 RASR */
    uint32_t rasr = 0;
    if (region->enable) {
        rasr |= xMPU_RASR_ENABLE;
    }
    rasr |= ((region->size & 0x1F) << xMPU_RASR_SIZE_Pos);
    rasr |= ((region->access & 0x07) << xMPU_RASR_AP_Pos);
    /* 设置 TEX/C/B 属性 (简化: 根据 attribute 枚举预设) */
    if (region->attribute == MPU_ATTR_DEVICE) {
        /* 设备: 强序或设备属性 */
        rasr |= (0x2 << xMPU_RASR_TEX_Pos); /* 设备模式, 无缓存 */
    } else {
        /* 普通内存: 可缓存、可缓冲 */
        rasr |= (0x1 << xMPU_RASR_C_Pos) | (0x1 << xMPU_RASR_B_Pos);
    }
    if (region->execute_never) {
        rasr |= (1 << xMPU_RASR_XN_Pos);
    }
    /* 子区域禁用掩码 */
    rasr |= ((region->subregion_disable & 0xFF) << xMPU_RASR_SRD_Pos);

    xMPU->RASR = rasr;
    __DSB();
    __ISB();

    return 0;
}

void hal_mpu_region_disable(uint8_t region_num)
{
    if (region_num >= MPU_MAX_REGIONS) {
        return;
    }
    xMPU->RNR = region_num;
    xMPU->RBAR = 0;
    xMPU->RASR = 0;
    __DSB();
    __ISB();
}

void mpu_switch_task_stack(uint32_t stack_base, mpu_region_size_t size) {
    const uint8_t STACK_REGION = 7;
    xMPU->RNR = STACK_REGION;
    xMPU->RBAR = (stack_base & 0xFFFFFFE0) | xMPU_RBAR_VALID | (STACK_REGION & 0xF);
    xMPU->RASR = xMPU_RASR_ENABLE
                | ((size & 0x1F) << xMPU_RASR_SIZE_Pos)
                | (MPU_ACCESS_PRIV_RW_USER_RO << xMPU_RASR_AP_Pos)  // 用户只读，特权读写
                | (1 << xMPU_RASR_XN_Pos);  // 禁止执行栈内代码
    __DSB();
    __ISB();
}

/* ===================================================================
   故障回调
   =================================================================== */
//void mpu_register_fault_callback(mpu_fault_callback_t callback)
//{
//    fault_cb = callback;
//}

/* ===================================================================
   MemManage 故障处理 (需要在中断向量表中注册)

   对照 MFSR 的位定义（从低位到高位）：
    位	名称	含义	您的值
    0	IACCVIOL	指令访问违例	0
    1	DACCVIOL	数据访问违例	1
    2	保留	-	0
    3	MUNSTKERR	异常返回时出栈违例	0
    4	MSTKERR	异常入栈时压栈违例	0
    5	MLSPERR	FPU延迟保存错误	0
    6	保留	-	0
    7	MMARVALID	MMFAR 有效	1

   =================================================================== */
void MemManage_Handler(void)
{
    uint32_t mmfar  = xSCB->MMFAR;
    uint32_t cfsr   = xSCB->CFSR;
    uint8_t mmfsr   = (cfsr >> 0) & 0xFF;
    uint8_t region = 0;
    fault_info_t fault = {0};
    // 读取故障状态
    /* 检查是否为 MPU 违规 (MMFSR.DACCVIOL 或 MMFSR.MUNSTKERR 等) */
    if ((mmfsr & 0x01) || (mmfsr & 0x02) || (mmfsr & 0x08)) {
        /* 尝试读取当前违规区域编号 (若 RNR 尚未被异常处理覆盖) */
        region = xMPU->RNR & 0x07;
        //if (fault_cb) {
        //    fault_cb(mmfar, region);
        //}
    } else {

        hal_fault_diag_decode(&fault);
    }

    /* 如果回调未处理，进入死循环或复位 */
    while (1) {
        __WFE();
    }
}

/*
 * STM32F407 关键存储区域定义
 * Flash:     0x08000000, 1MB
 * SRAM:      0x20000000, 128KB
 * Peripheral:0x40000000, 1MB
 * External:  0x60000000, 1MB (FSMC)
 * System:    0xE0000000, 1MB (NVIC, SCB, MPU 等)
 * Backup SRAM:0x40024000, 4KB (可选，如果使用 RTC 备份域)
 */


/* 区域 6~7 保留给动态任务栈保护，不在此处配置 */
const mpu_config_t mpu_cfg = {
        .enable_default_map = true,      // PRIVDEFENA = 1，特权模式背景区域
        .enable_background  = true,      // 使能背景区域
        .num_regions        = 6,         // 使用了 6 个静态区域
        .regions            = {
                &(const mpu_region_config_t){/*区域 0: Flash 代码区 特权/用户只读，可执行*/
                        .region_num       = 0,
                        .enable           = true,
                        .base_address     = 0x08000000,
                        .size             = MPU_SIZE_1M,
                        .access           = MPU_ACCESS_PRIV_RO_USER_RO,  /* 读写均只读，防止意外擦写 */
                        .attribute        = MPU_ATTR_NORMAL,
                        .execute_never    = false,                         /* Flash 必须可执行 */
                        .subregion_disable = 0
                },
                &(const mpu_region_config_t){/* 区域 1: SRAM (数据区) —— 特权可读写，用户可读写 */
                        .region_num       = 1,
                        .enable           = true,
                        .base_address     = 0x20000000,
                        .size             = MPU_SIZE_128K,  /* 如需保护所有 SRAM (包括可能的后128KB) 可用 MPU_SIZE_256K */
                        .access           = MPU_ACCESS_FULL, /* MPU_ACCESS_PRIV_RW 特权读写，用户禁止/特权与用户均可读写 */
                        .attribute        = MPU_ATTR_NORMAL,
                        .execute_never    = true,            /* 禁止在 SRAM 中执行代码 (防御 ROP) */
                        .subregion_disable = 0
                },
                &(const mpu_region_config_t){/* 区域 2: 片内外设区 驱动层—— 特权可读写，用户无访问权限 */
                        .region_num       = 2,
                        .enable           = true,
                        .base_address     = 0x40000000,
                        .size             = MPU_SIZE_1M,
                        .access           = MPU_ACCESS_FULL,//MPU_ACCESS_PRIV_RW,  /* 用户不能直接操作外设 */
                        .attribute        = MPU_ATTR_DEVICE,     /* 设备属性，禁止缓存 */
                        .execute_never    = true,                /* 外设寄存器不可执行 */
                        .subregion_disable = 0
                },
                &(const mpu_region_config_t){/* 区域 3: 外部存储器区 (FSMC) —— 特权可读写，用户可读写 */
                        .region_num       = 3,
                        .enable           = true,
                        .base_address     = 0x60000000,
                        .size             = MPU_SIZE_1M,
                        .access           = MPU_ACCESS_FULL,
                        .attribute        = MPU_ATTR_DEVICE,     /* 如果是 NOR/SRAM 可用 NORMAL */
                        .execute_never    = true,
                        .subregion_disable = 0
                },
                &(const mpu_region_config_t){/* 区域 4: 系统区 (SCB/NVIC/MPU 等) —— 特权可读写，用户不可访问 */
                        .region_num       = 4,
                        .enable           = true,
                        .base_address     = 0xE0000000,
                        .size             = MPU_SIZE_1M,
                        .access           = MPU_ACCESS_PRIV_RW,//MPU_ACCESS_PRIV_RW,
                        .attribute        = MPU_ATTR_STRONGLY_ORDERED, /* 强序，保证执行顺序 */
                        .execute_never    = true,                       /* 内核寄存器不可执行 */
                        .subregion_disable = 0
                },
                &(const mpu_region_config_t){/* 区域 5: 备份 SRAM (可选) —— 若使用备份域则保护 */
                        .region_num       = 5,
                        .enable           = true,
                        .base_address     = 0x40024000,
                        .size             = MPU_SIZE_4K,
                        .access           = MPU_ACCESS_PRIV_RW,
                        .attribute        = MPU_ATTR_NORMAL,
                        .execute_never    = true,
                        .subregion_disable = 0
                }
        }
};

void system_mpu_setup(void)
{
    mpu_init(&mpu_cfg);
    //mpu_register_fault_callback(my_mpu_fault_handler);
}
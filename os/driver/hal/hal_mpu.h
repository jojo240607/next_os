//
// Created by zhiwei.gong on 2026/5/21.
//

#ifndef STM32F4DISCOVERY_HAL_MPU_H
#define STM32F4DISCOVERY_HAL_MPU_H

#include <stdint.h>
#include <stdbool.h>

/* MPU 区域数量 (Cortex-M4) */
#define MPU_MAX_REGIONS     8

/* 区域大小 (2^N, N >= 5 且 N <= 31) */
typedef enum {
    MPU_SIZE_0B     = 0,
    MPU_SIZE_32B    = 5,
    MPU_SIZE_64B    = 6,
    MPU_SIZE_128B   = 7,
    MPU_SIZE_256B   = 8,
    MPU_SIZE_512B   = 9,
    MPU_SIZE_1K     = 10,
    MPU_SIZE_2K     = 11,
    MPU_SIZE_4K     = 12,
    MPU_SIZE_8K     = 13,
    MPU_SIZE_16K    = 14,
    MPU_SIZE_32K    = 15,
    MPU_SIZE_64K    = 16,
    MPU_SIZE_128K   = 17,
    MPU_SIZE_256K   = 18,
    MPU_SIZE_512K   = 19,
    MPU_SIZE_1M     = 20,
    MPU_SIZE_2M     = 21,
    MPU_SIZE_4M     = 22,
    MPU_SIZE_8M     = 23,
    MPU_SIZE_16M    = 24,
    MPU_SIZE_32M    = 25,
    MPU_SIZE_64M    = 26,
    MPU_SIZE_128M   = 27,
    MPU_SIZE_256M   = 28,
    MPU_SIZE_512M   = 29,
    MPU_SIZE_1G     = 30,
    MPU_SIZE_2G     = 31
} mpu_region_size_t;

/* 区域访问权限 */
typedef enum {
    MPU_ACCESS_NO      = 0,    /* 无访问权限 */
    MPU_ACCESS_PRIV_RW = 1,    /* 特权模式可读写 */
    MPU_ACCESS_PRIV_RW_USER_RO = 2, /* 特权可读写, 用户只读 */
    MPU_ACCESS_FULL     = 3,   /* 特权与用户均可读写 */
    MPU_ACCESS_PRIV_RO  = 5,   /* 特权只读 */
    MPU_ACCESS_PRIV_RO_USER_RO = 6, /* 特权与用户均只读 */
    /* 注：其他编码见 ARMv7-M 手册 */
} mpu_access_permission_t;

/* 区域属性 (可缓冲/可缓存/可共享) */
typedef enum {
    MPU_ATTR_NORMAL    = 0,    /* 内部存储 (普通) */
    MPU_ATTR_DEVICE    = 1,    /* 外设/设备 */
    MPU_ATTR_STRONGLY_ORDERED = 2, /* 强序 */
    /* 简化：通常只用 0 或 1 */
} mpu_region_attr_t;

/* MPU 区域配置描述符 */
typedef struct {
    uint8_t                 region_num;     /* 区域编号 0~7 */
    bool                    enable;         /* 是否使能该区域 */
    uint32_t                base_address;   /* 基地址 (必须对齐到大小) */
    mpu_region_size_t       size;           /* 区域大小 */
    mpu_access_permission_t access;         /* 访问权限 */
    mpu_region_attr_t       attribute;      /* 内存属性 */
    bool                    execute_never;  /* 禁止执行 (XN) */
    uint8_t                 subregion_disable; /* 子区域禁用掩码 (8位, 每位置1禁用对应子区域) */
} mpu_region_config_t;

/* MPU 总配置描述符 */
typedef struct {
    bool                    enable_default_map; /* 使能默认内存映射 (背景区域) 仅特权模式下 */
    bool                    enable_background;  /* 使能背景区域 (MPU_CTRL_PRIVDEFENA) */
    uint8_t                 num_regions;        /* 本次配置的区域个数 */
    const mpu_region_config_t * const regions[];         /* 区域配置数组 */
} mpu_config_t;

/* ========== API ========== */
int  hal_mpu_init(const mpu_config_t *cfg);
void hal_mpu_deinit(void);              /* 禁用 MPU 回到默认 */

void hal_mpu_enable(void);
void hal_mpu_disable(void);

/* 单独配置/修改一个区域 */
int  hal_mpu_region_set(const mpu_region_config_t *region);
void hal_mpu_region_disable(uint8_t region_num);
void mpu_switch_task_stack(uint32_t stack_base, mpu_region_size_t size);
void system_mpu_setup(void);
/* 故障处理回调 */
//typedef void (*mpu_fault_callback_t)(uint32_t fault_addr, uint8_t region_num);
//void hal_mpu_register_fault_callback(mpu_fault_callback_t callback);
void MemManage_Handler(void);
#endif //STM32F4DISCOVERY_HAL_MPU_H

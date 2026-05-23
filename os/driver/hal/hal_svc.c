//
// Created by zhiwei.gong on 2026/5/22.
//

#include "hal_svc.h"
#include <stddef.h>

/* ── 系统调用表（静态存储） ── */
static svc_handler_t svc_table[SVC_MAX_ID] = {NULL};
//static bool svc_initialized = false;

/* ===================================================================
   初始化
   =================================================================== */
int svc_init(const svc_config_t *cfg)
{
    if (!cfg) return -1;

    /* 清空表 */
    for (int i = 0; i < SVC_MAX_ID; i++) {
        svc_table[i] = NULL;
    }

    /* 注册所有条目 */
    for (uint32_t i = 0; i < cfg->num_entries; i++) {
        uint32_t id = cfg->entries[i].number;
        if (id < SVC_MAX_ID) {
            svc_table[id] = cfg->entries[i].handler;
        }
    }

    //svc_initialized = true;
    return 0;
}

void svc_deinit(void)
{
    for (int i = 0; i < SVC_MAX_ID; i++) {
        svc_table[i] = NULL;
    }
    //svc_initialized = false;
}

/* ===================================================================
   系统调用分发
   =================================================================== */
uint32_t svc_dispatch(svc_number_t svc_num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
{
    if (svc_num >= SVC_MAX_ID || svc_table[svc_num] == NULL) {
        return (uint32_t)-1;   /* 无效系统调用号 */
    }
    return svc_table[svc_num](a1, a2, a3, a4);
}

/* ===================================================================
   SVC 异常处理程序（汇编 + C 集成）
   =================================================================== */

/*
 * SVC_Handler 采用 naked 属性，手动处理上下文，
 * 提取系统调用号和参数，调用 svc_dispatch，再将返回值写回任务栈。
 */

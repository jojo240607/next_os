//
// Created by zhiwei.gong on 2026/5/29.
//
// 任务配置表 —— 所有系统任务和测试任务在此注册。
// 生产环境中建议用宏控制测试任务的编译：
//   #ifdef ENABLE_TEST_TASKS → 包含 test/ 下的任务
//   #else → 仅编译系统任务

#include "task_config.h"
#include "../task/os/systick_task.h"
#include "../task/os/os_cb_task.h"
#include "../task/os/idle_task.h"
#include "../task/os/monitor_task.h"
#include "../task/os/log_task.h"
#include "../test/time_task.h"
#include "../test/task_test1.h"
#include "../test/uart_dma_test.h"
#include "../kernel/kworker.h"

static const task_into_t systick_task_info = {
        .name = "systick",
        .priority = THREAD_PRIORITY_1,
        .stack_size = MPU_SIZE_0B,
        .create = (Task_create)systick_task_create
};

static const task_into_t os_cb_task_info = {
        .name = "os_cb",
        .priority = THREAD_PRIORITY_5,
        .stack_size = MPU_SIZE_1K,
        .create = (Task_create)os_cb_task_create
};

static const task_into_t idle_task_info = {
        .name = "idle",
        .priority = THREAD_PRIORITY_0,
        .stack_size = MPU_SIZE_128B,
        .create = (Task_create)idle_task_create
};

static const task_into_t monitor_task_info = {
        .name = "monitor",
        .priority = THREAD_PRIORITY_0,
        .stack_size = MPU_SIZE_1K,
        .create = (Task_create)monitor_task_create
};

static const task_into_t log_task_info = {
        .name = "log",
        .priority = THREAD_PRIORITY_0,
        .stack_size = MPU_SIZE_1K,
        .create = (Task_create)log_task_create
};

static const task_into_t time_task_info = {
        .name = "time",
        .priority = THREAD_PRIORITY_1,
        .stack_size = MPU_SIZE_1K,
        .create = (Task_create)time_task_create
};

static const task_into_t test1_task_info = {
        .name = "test1",
        .priority = THREAD_PRIORITY_1,
        .stack_size = MPU_SIZE_1K,
        .create = (Task_create)task_test1_create
};

static const task_into_t uart_dma_task_info = {
        .name = "uart_dma",
        .priority = THREAD_PRIORITY_2,
        .stack_size = MPU_SIZE_2K,
        .create = (Task_create)uart_dma_test_create
};
//内核态工作队列
static const task_into_t kworker_task_info = {
        .name = "kworker",
        .priority = THREAD_PRIORITY_7,
        .stack_size = MPU_SIZE_512B,
        .create = (Task_create)kworker_create,
        .is_kernel = true
};

const task_into_t * const task_lists[] = {
        (const task_into_t *)(&kworker_task_info),
        (const task_into_t *)(&os_cb_task_info),
        (const task_into_t *)(&idle_task_info),
        (const task_into_t *)(&monitor_task_info),
        (const task_into_t *)(&log_task_info),
        (const task_into_t *)(&test1_task_info),
        (const task_into_t *)(&time_task_info),
        (const task_into_t *)(&systick_task_info),
        (const task_into_t *)(&uart_dma_task_info),
};

const size_t totel_task = ARRAY_SIZE(task_lists);

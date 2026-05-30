//
// Created by zhiwei.gong on 2026/5/29.
//

#include "task_config.h"
#include "../task/os/systick_task.h"
#include "../task/os/os_cb_task.h"
#include "../task/os/idle_task.h"
#include "../task/os/monitor_task.h"
#include "../task/os/log_task.h"
#include "../test/time_task.h"
#include "../test/task_test1.h"

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

const task_into_t * const task_lists[] = {
        (const task_into_t *)(&os_cb_task_info),
        (const task_into_t *)(&idle_task_info),
        (const task_into_t *)(&monitor_task_info),
        (const task_into_t *)(&log_task_info),
        (const task_into_t *)(&test1_task_info),
        (const task_into_t *)(&time_task_info),
        (const task_into_t *)(&systick_task_info),
};

const size_t totel_task = ARRAY_SIZE(task_lists);

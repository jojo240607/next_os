// file: port.s
    .syntax unified
    .thumb
    .text

    .global PendSV_Handler          // 导出符号，覆盖弱定义
    .global pxCurrentTCB            // 引用C文件中定义的当前TCB指针

    .type   PendSV_Handler, %function
PendSV_Handler:
    // 进入PendSV时，CPU已自动将 xPSR, PC, LR, R12, R0-R3 压入当前任务的堆栈
    // 我们需要手动保存剩余的 R4-R11

    // 1. 获取当前任务的PSP
    mrs     r0, psp
    // 2. 保存R4-R11到当前任务堆栈
    stmdb   r0!, {r4-r11}

    // 3. 将新的栈顶指针保存到当前任务的TCB中
    ldr     r1, =pxCurrentTCB
    ldr     r2, [r1]        // r2 = pxCurrentTCB (TCB指针)
    str     r0, [r2]        // TCB->stack_ptr = 新栈顶

    // 4. 调用C调度器，选择下一个要运行的任务
    //    注意：C函数 vTaskSwitchContext 会修改 pxCurrentTCB
    bl      vTaskSwitchContext

    // 5. 恢复新任务的上下文
    ldr     r1, =pxCurrentTCB
    ldr     r2, [r1]        // r2 = 新任务的TCB指针
    ldr     r0, [r2]        // r0 = 新任务的栈顶指针
    ldmia   r0!, {r4-r11}   // 从新任务堆栈恢复 R4-R11
    msr     psp, r0         // 更新PSP为新任务的栈顶

    // 6. 退出中断（使用 bx lr，CPU自动从PSP弹出剩余寄存器）
    bx      lr

    .size   PendSV_Handler, .-PendSV_Handler
    .end
// file: context_switch.s
    .syntax unified
    .thumb
    .text

    .global PendSV_Handler          // 导出符号，覆盖弱定义
    .global gloable_current_stack            // 引用C文件中定义的当前TCB Stack指针

    .type   PendSV_Handler, %function
PendSV_Handler:
    // 进入PendSV时，CPU已自动将 xPSR, PC, LR, R12, R0-R3 压入当前任务的堆栈
    // 我们需要手动保存剩余的 R4-R11

    // 1. 获取当前任务的PSP
    mrs     r0, psp
    // 2. 保存R4-R11到当前任务堆栈
    stmdb   r0!, {r4-r11} //地址递减

    // 3. 将新的栈顶指针保存到当前任务的TCB中
    ldr     r1, =gloable_current_stack
    ldr     r2, [r1]        // r2 = gloable_current_stack (TCB Stack指针)
    str     r0, [r2]        // TCB->stack_ptr = 新栈顶
    mov r4, lr          // 保存 EXC_RETURN 到 r4（r4 会被手动保存）
    // 4. 调用C调度器，选择下一个要运行的任务
    //    注意：C函数 vTaskSwitchContext 会修改 gloable_current_stack
    ldr     r0, =global_thread_scheduler   // 加载全局变量的地址（如果参数是指针）
    ldr     r0, [r0]               // 取出值（如果参数是数值）
    bl      thread_scheduler_switch_context
    mov lr, r4          // 恢复 EXC_RETURN
    // 5. 恢复新任务的上下文
    ldr     r1, =gloable_current_stack
    ldr     r2, [r1]        // r2 = 新任务的TCB指针
    ldr     r0, [r2]        // r0 = 新任务的栈顶指针
    ldmia   r0!, {r4-r11}   // 从新任务堆栈恢复 R4-R11  地址递增
    msr     psp, r0         // 更新PSP为新任务的栈顶

    // 6. 退出中断（使用 bx lr，CPU自动从PSP弹出剩余寄存器）
    bx      lr

    .size   PendSV_Handler, .-PendSV_Handler
    .end
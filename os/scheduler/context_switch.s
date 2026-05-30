// file: context_switch.s
    .syntax unified
    .thumb
    .text

    .global PendSV_Handler          // 导出符号，覆盖弱定义
    .global gloable_current_tcb            // 引用C文件中定义的当前TCB Stack指针

    .equ  SP_OFFSET,         64               // TCB->sp 的偏移（字节）
    .equ  IRQ_PREEMPT_PRIORITY_SYSCALL,    0x50// 5 << 4 IRQ_PREEMPT_PRIORITY_SYSCALL
    .type   PendSV_Handler, %function
PendSV_Handler:
    // 进入PendSV时，CPU已自动将 xPSR, PC, LR, R12, R0-R3 压入当前任务的堆栈
    // 我们需要手动保存剩余的 R4-R11 R14(LR)

    // 1. 获取当前任务的PSP
    mrs     r0, psp
    isb
    tst lr, #0x10        // ───── ③ 检查是否使用过 FPU
    it eq
    vstmdbeq r0!, {s16-s31} // ── ④ 手动保存高 16 个 FPU 寄存器
    // 2. 保存R4-R11, lr, control到当前任务堆栈
    mrs     r12, control
    stmdb   r0!, {r4-r12, lr} //地址递减 保存内核寄存器 + EXC_RETURN
    // 3. 将新的栈顶指针保存到当前任务的TCB中
    ldr     r1, =gloable_current_tcb
    ldr     r2, [r1]                    // r2 = gloable_current_tcb (TCB Stack指针)
    str     r0, [r2, #SP_OFFSET]        // TCB->sp = 新栈顶 更新当前 TCB 栈顶指针
     // ************ 刷新所有未完成的内存访问 ************
    dsb                           // 确保之前的所有写操作都已完成
    isb                           // 同步指令流，保证后续指令看到最新状态
    //保护现场 设置中断屏蔽 IRQ_PREEMPT_PRIORITY_SYSCALL
    stmdb sp!, {r0, r1}   // ───── ⑦ 保护现场 r0 r1
    mov r0, #IRQ_PREEMPT_PRIORITY_SYSCALL
    msr basepri, r0       // 提升中断屏蔽级别
    dsb
    isb
    // 4. 调用C调度器，选择下一个要运行的任务
    //    注意：C函数 vTaskSwitchContext 会修改 gloable_current_tcb
    ldr     r0, =global_thread_scheduler   // 加载全局变量的地址（如果参数是指针）
    ldr     r0, [r0]               // 取出值（如果参数是数值）
    bl      thread_scheduler_switch_context
    //恢复中断屏蔽
    mov r0, #0
    msr basepri, r0       // 恢复中断屏蔽
    ldmia sp!, {r0, r1}   // 恢复 r0, r1
    // 5. 恢复新任务的上下文
    ldr     r2, [r1]        // r2 = 新任务的TCB指针
    // 6. 恢复新任务的 PSP 和 R4-R11
    ldr     r0, [r2, #SP_OFFSET]        // r0 = 新任务的栈顶指针
    ldmia   r0!, {r4-r12, lr}   // 从新任务堆栈恢复 R4-R11  R14 control地址递增

    tst lr, #0x10        // ───── ⑪ 检查新任务是否使用过 FPU
    it eq
    vldmiaeq r0!, {s16-s31} // ── ⑫ 恢复高 16 个 FPU 寄存器

    msr     psp, r0         // 更新PSP为新任务的栈顶
    msr     control, r12          // 恢复 CONTROL（SP 可能立刻切换）
    isb                            //指令同步屏障，确保切换完成
    // 6. 退出中断（使用 bx lr，CPU自动从PSP弹出剩余寄存器）
    bx      lr

    .size   PendSV_Handler, .-PendSV_Handler
    .end
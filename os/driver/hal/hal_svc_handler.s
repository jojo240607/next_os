// file: svc_handler.S
    .syntax unified
    .thumb
    .section .text
    .global SVC_Handler
    .type   SVC_Handler, %function

SVC_Handler:
    /* 1. 保存 R4-R11 到任务栈（CPU 已自动压栈 R0-R3,R12,LR,PC,xPSR） */
    mrs     r0, psp             // r0 = 自动压栈后的栈顶
    isb
    mov     r12, r0             // 备份
    tst lr, #0x10        // ───── ③ 检查是否使用过 FPU
    it eq
    vstmdbeq r0!, {s16-s31} // ── ④ 手动保存高 16 个 FPU 寄存器
    stmdb   r0!, {r4-r11}       // 手动压栈（会破坏 r0）
    msr     psp, r0

    /* 2. 获取自动压栈的栈帧基址 */
    mov     r0, r12
    /* 此时 r0 指向自动压栈的栈顶，即 R0 存放的位置 */

    /* 3. 提取系统调用号 (R0) 和参数 (R1-R3) */
    ldr     r1, [r0, #0]        /* r1 = svc_num (原 R0) */
    ldr     r2, [r0, #4]        /* r2 = 参数1 (原 R1) */
    ldr     r3, [r0, #8]        /* r3 = 参数2 (原 R2) */
    ldr     r4, [r0, #12]       /* r4 = 参数3 (原 R3) */

    /* 4. 调用 C 分发函数：svc_dispatch(svc_num, a1, a2, a3, 0) */
    push    {lr}                /* 保存 EXC_RETURN */
    mov     r0, r1              /* r0 = svc_num */
    mov     r1, r2              /* r1 = a1 */
    mov     r2, r3              /* r2 = a2 */
    mov     r3, r4              /* r3 = a3 */
    mov     r4, #0              /* r4 未使用，传 0 */
    bl      svc_dispatch
    pop     {lr}                /* 恢复 EXC_RETURN */

    /* 5. 将返回值写入任务栈的 R0 位置 */
    /*    首先找到自动压栈的 R0 地址：当前 PSP 指向 R4 之后，需要加上 32 字节 */
    mrs     r1, psp
    add     r1, r1, #32         /* PSP + 32 = 自动压栈的 R0 地址 */
    str     r0, [r1, #0]        /* 将返回值写入栈中的 R0 */

    /* 6. 恢复 R4-R11 并异常返回 */
    mrs     r0, psp
    ldmia   r0!, {r4-r11}
    tst lr, #0x10        // ───── ⑪ 检查新任务是否使用过 FPU
    it eq
    vldmiaeq r0!, {s16-s31} // ── ⑫ 恢复高 16 个 FPU 寄存器
    msr     psp, r0
    bx      lr

    .size   SVC_Handler, .-SVC_Handler
    .end
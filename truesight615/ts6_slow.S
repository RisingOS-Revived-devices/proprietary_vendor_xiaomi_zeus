// SPDX-FileCopyrightText: 2026 The LineageOS Project
// SPDX-License-Identifier: Apache-2.0
//
// Slow path of the ts6 trampolines: x17 holds the slot index. Save the
// argument registers, load the driver, then jump to the resolved function.

.text
.globl ts6_slow
.hidden ts6_slow
.type ts6_slow,%function
ts6_slow:
    bti c
    stp x29, x30, [sp, #-224]!
    mov x29, sp
    stp x0, x1, [sp, #16]
    stp x2, x3, [sp, #32]
    stp x4, x5, [sp, #48]
    stp x6, x7, [sp, #64]
    stp q0, q1, [sp, #80]
    stp q2, q3, [sp, #112]
    stp q4, q5, [sp, #144]
    stp q6, q7, [sp, #176]
    stp x8, x17, [sp, #208]
    bl ts6_resolve
    ldp x8, x17, [sp, #208]
    ldp q6, q7, [sp, #176]
    ldp q4, q5, [sp, #144]
    ldp q2, q3, [sp, #112]
    ldp q0, q1, [sp, #80]
    ldp x6, x7, [sp, #64]
    ldp x4, x5, [sp, #48]
    ldp x2, x3, [sp, #32]
    ldp x0, x1, [sp, #16]
    ldp x29, x30, [sp], #224
    adrp x16, ts6_ptrs
    add x16, x16, :lo12:ts6_ptrs
    ldr x16, [x16, x17, lsl #3]
    br x16
.size ts6_slow, .-ts6_slow

// BTI/PAC-compatible object, required by the platform's -z bti-report=error
.pushsection .note.gnu.property, "a"
.balign 8
.long 4
.long 16
.long 5
.asciz "GNU"
.long 0xc0000000
.long 4
.long 3
.long 0
.popsection

.text
.global main
add:
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  mov x0, x0
  str x0, [sp, #-16]!
  mov x0, x1
  mov x1, x0
  ldr x0, [sp], #16
  add x0, x0, x1
  ldp x29, x30, [sp], #16
  ret

main:
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  adrp x0, .LA0
  add x0, x0, :lo12:.LA0
  mov x2, x0
  mov x0, x2
  str x0, [sp, #-16]!
  mov x0, #1
  lsl x0, x0, #3
  ldr x1, [sp], #16
  ldr x0, [x1, x0]
  mov x3, x0
  mov x0, #2
  str x0, [sp, #-16]!
  mov x0, #2
  mov x1, x0
  ldr x0, [sp], #16
  cmp x0, x1
  cset x0, eq
  mov x4, x0
  mov x0, #1
  mov x0, x0
  adrp x0, .LC1
  add x0, x0, :lo12:.LC1
  mov x1, x0
  mov x0, #3
  mov x2, x0
  bl sys_write
  mov x0, x4
  cmp x0, #0
  b.eq .Lelse0
  mov x0, #20
  mov x0, x0
  mov x0, #22
  mov x1, x0
  bl add
  b .Lend0
  .Lelse0:
  mov x0, #0
  .Lend0:
  ldp x29, x30, [sp], #16
  ret

sys_write:
  mov x8, #64
  svc #0
  ret
.section .rodata
.LA0:
  .quad 7
  .quad 9
.LC1: .asciz "ok\n"
typecheck: PASS

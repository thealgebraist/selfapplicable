normalized return: 141
.text
.global main
main:
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  mov x0, #141
  ldp x29, x30, [sp], #16
  ret

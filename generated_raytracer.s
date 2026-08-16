.section .text
.global _start
_start:
  lea buffer(%rip), %r8
  xor %r12d, %r12d
.Lrow:
  cmp $64, %r12d
  jge .Lopen
  xor %r13d, %r13d
.Lcol:
  cmp $96, %r13d
  jge .Lnextrow
  mov %r13d, %eax
  sub $48, %eax
  imul %eax, %eax
  mov %r12d, %ecx
  sub $32, %ecx
  imul %ecx, %ecx
  add %ecx, %eax
  cmp $576, %eax
  setl %al
  neg %al
  and $255, %al
  mov %al, (%r8)
  inc %r8
  inc %r13d
  jmp .Lcol
.Lnextrow:
  inc %r12d
  jmp .Lrow
.Lopen:
  mov $257, %eax
  mov $-100, %edi
  lea path(%rip), %rsi
  mov $577, %edx
  mov $0644, %r10d
  syscall
  mov %eax, %r12d
  mov $1, %eax
  mov %r12d, %edi
  lea header(%rip), %rsi
  mov $13, %edx
  syscall
  mov $1, %eax
  mov %r12d, %edi
  lea buffer(%rip), %rsi
  mov $6144, %edx
  syscall
  mov $3, %eax
  mov %r12d, %edi
  syscall
  mov $60, %eax
  xor %edi, %edi
  syscall
.section .rodata
header: .ascii "P5\n96 64\n255\n"
path: .asciz "render.pgm"
.section .bss
.lcomm buffer, 6144

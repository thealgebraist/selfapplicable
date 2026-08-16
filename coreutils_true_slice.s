.text
.globl _start
_start:
  mov (%rsp), %rdi
  cmp $2, %rdi
  jne .Lsuccess
  mov 16(%rsp), %rsi
  cmpb $'-', (%rsi)
  jne .Lsuccess
  cmpb $'-', 1(%rsi)
  jne .Lsuccess
  cmpb $'h', 2(%rsi)
  je .Lsuccess
  cmpb $'v', 2(%rsi)
  je .Lsuccess
.Lsuccess:
  xor %edi, %edi
  jmp .Lexit
.Lexit:
  mov $60, %eax
  syscall

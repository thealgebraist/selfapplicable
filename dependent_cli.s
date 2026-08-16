.text
.globl _start
_start:
  mov $0, %edi
  mov $60, %eax
  syscall

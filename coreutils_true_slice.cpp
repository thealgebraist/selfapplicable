// A first freestanding Coreutils compatibility slice.
// It models the observable core of true(1): inspect argc/argv and return an
// exit status.  The backend emits x86-64 Linux assembler; no C compiler or
// libc is involved when the generated program is assembled and linked.
#include <iostream>

int main() {
  std::cout << ".text\n"
               ".globl _start\n"
               "_start:\n"
               "  mov (%rsp), %rdi\n"       // argc
               "  cmp $2, %rdi\n"
               "  jne .Lsuccess\n"
               "  mov 16(%rsp), %rsi\n"      // argv[1]
               "  cmpb $'-', (%rsi)\n"
               "  jne .Lsuccess\n"
               "  cmpb $'-', 1(%rsi)\n"
               "  jne .Lsuccess\n"
               "  cmpb $'h', 2(%rsi)\n"
               "  je .Lsuccess\n"
               "  cmpb $'v', 2(%rsi)\n"
               "  je .Lsuccess\n"
               ".Lsuccess:\n"
               "  xor %edi, %edi\n"
               "  jmp .Lexit\n"
               ".Lexit:\n"
               "  mov $60, %eax\n"
               "  syscall\n";
}

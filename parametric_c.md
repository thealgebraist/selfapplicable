# Parametric C-variant compiler

`parametric_c.cpp` makes the target compiler a function of a high-level
semantic description:

```cpp
struct CDescription {
  std::string name;
  unsigned integer_bits;
  std::string entry;
  std::string result_register;
  std::string syscall_number_register;
  std::int64_t write_syscall;
  std::string syscall_instruction;
};
```

The compiler normalizes the source expression first, then lowers it using the
description. The description itself is a typed code value:

```cpp
auto quoted = quote_description(linuxC);
auto staged = compile_via_staged_generator(quoted, source);
```

Direct compilation and compilation through the quoted semantic description are
compared byte-for-byte. The sample normalizes `40 + (2 + 1)` to `43` and emits:

```asm
.text
.global main
main:
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  mov x0, #43
  ldp x29, x30, [sp], #16
  ret
```

Build:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 parametric_c.cpp -o parametric_c
./parametric_c
```

# Compact Python-like to AArch64 compiler

`pyarm64.cpp` is a small compiler for:

```text
let name = expression;
...
return expression;
```

Expressions support integer literals, names, parentheses, `+`, `-`, and `*`.

The pipeline is:

1. Parse into an immutable expression AST.
2. Normalize with a constant environment, substituting known `let` bindings and folding arithmetic.
3. Remove constant `let` statements from the residual program.
4. Emit a conservative AArch64 function using `x0`, `x1`, `x29`, and `x30`.

Build:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 pyarm64.cpp -o pyarm64
./pyarm64 > sample_arm64.s
```

For the sample source:

```text
let x = 2 + 3 * 4;
let y = x * 10;
return y + 1;
```

the normalizer produces `141`, and the residual assembly is:

```asm
.text
.global main
main:
  stp x29, x30, [sp, #-16]!
  mov x29, sp
  mov x0, #141
  ldp x29, x30, [sp], #16
  ret
```

This is a compiler prototype, not a complete Python implementation: it has no
objects, calls, exceptions, mutation, or runtime library. The emitted assembly
is intended for AArch64 assemblers and is structurally checked on the current
host.

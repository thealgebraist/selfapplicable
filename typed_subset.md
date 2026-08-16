# Strongly typed C-like subset to AArch64

`typed_subset.cpp` extends the compact compiler without adding dynamic or
unusual language features. It supports a typed AST for:

- `int`, `bool`, `string`, fixed-size integer arrays, and `void`;
- function definitions and typed parameters;
- `if` expressions;
- arithmetic, equality, boolean values, arrays, indexing, and strings;
- explicit `sys_write` lowering to the Linux AArch64 syscall ABI;
- a simple deterministic register assignment (`x0`–`x2`) with stack spills for binary expressions.

The checker runs before lowering. The normalizer folds static integer and
boolean expressions, and code generation preserves the checked result types.

Build and inspect the generated assembly:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 typed_subset.cpp -o typed_subset
./typed_subset > typed_subset.s
```

The executable reports `typecheck: PASS` and emits `add`, `main`, and the
explicit `sys_write` syscall helper, together with string and array data in
`.rodata`.

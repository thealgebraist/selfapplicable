# Coreutils source compatibility audit

The upstream GNU coreutils source was downloaded into `coreutils-src/` at
revision `d458e11`.

Representative source sizes:

```text
true.c  73 lines
echo.c 286 lines
pwd.c  386 lines
```

The current typed compiler cannot yet compile these files directly. Even the
smallest `true.c` uses generated `config.h`, project headers, `argc`/`argv`
pointers, structs, libc calls, option parsing, and localization helpers. The
`echo.c` and `pwd.c` implementations additionally use pointer arithmetic,
variadic/libc I/O, global configuration, and project-specific allocation and
diagnostic APIs.

The correct compatibility boundary is therefore:

1. preprocess macros and generated configuration outside the typed compiler;
2. translate the resulting C AST into the compiler's typed IR;
3. support pointers, arrays, structs, function calls, and explicit libc/syscall
   effects in the IR;
4. lower the IR using the documented machine ABI;
5. link only against an explicitly declared runtime surface.

The source checkout is ready for incremental work, but claiming that the
current small compiler builds GNU `ls`, `find`, or `echo` would be false. The
first realistic milestone is a freestanding, dependency-reduced `true` or
`echo` compatibility slice before attempting the full utilities.

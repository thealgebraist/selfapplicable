# Macro-free C subset

The first source-ingestion slice is deliberately explicit. It accepts a
translation unit containing one function with the shape:

```c
int main(int argc, char **argv) { return DECIMAL; }
```

The frontend also accepts the first control-flow form:

```c
int main(int argc, char **argv) {
  if (argc == DECIMAL) return DECIMAL;
  return DECIMAL;
}
```

`argc` is read from the Linux process-entry stack and lowered to a compare
and branch in the generated assembler.

The subset also accepts one literal write statement:

```c
write(1, "echo\\n", 5);
```

It is lowered directly to Linux `write(2)` syscall number 1, with the literal
stored in `.rodata`. C string escape `\\n` is currently supported.

Argument output is also supported:

```c
write(1, argv[1], strlen(argv[1]));
```

The compiler lowers `argv[1]` from the Linux entry stack and emits a bytewise
NUL scan before the syscall. Missing `argv[1]` produces an empty successful
command in this initial slice.

The option guard used by `true(1)` is recognized in this reduced form:

```c
if (argc == 2 && streq(argv[1], "--help")) return 0;
```

The backend emits bounded byte comparisons for the literal option. General
pointer arithmetic and arbitrary function calls remain outside the subset.

Bounded counted loops are supported for literal output:

```c
for (int i = 0; i < 3; i++) write(1, "x\\n", 2);
```

The loop counter is lowered to a register that survives Linux `syscall`
clobbers.

The first filesystem primitive is the `pwd` pattern:

```c
char buf[4096];
write(1, getcwd(buf, 4096), strlen(buf));
```

It lowers to Linux `getcwd` (syscall 79), a NUL scan, and `write`. The buffer
declaration is recognized as a fixed compiler-owned storage region.

Directory traversal is available through the first filesystem builtin:

```c
listdir(".");
```

This lowers to `openat`, `getdents64`, Linux `dirent64` record-length/name
decoding, and one `write` per entry. It is the backend boundary used by the
reduced `ls` fixture; recursive `find` traversal is not yet implemented.

The same one-level primitive is exposed as:

```c
finddir(".");
```

This compiles and emits the entries in the selected directory. Recursive
descent remains a future typed-library feature.

Exact-name filtering is now supported for one-level traversal:

```c
finddir(".", "normaliser.cpp");
```

The generated code compares the decoded entry name byte-by-byte before
emitting it.

Filesystem existence predicates are supported with:

```c
if (exists("normaliser.cpp")) return 0;
return 1;
```

The compiler lowers `exists` to Linux `statx` and branches on its result.

Directory-kind predicates are supported with:

```c
if (isdir(".")) return 0;
return 1;
```

The generated code checks the `statx` mode field against `S_IFDIR`.

Regular-file predicates use the corresponding form:

```c
if (isreg("normaliser.cpp")) return 0;
return 1;
```

The backend checks `S_IFREG` from the same `statx` mode field.

Size predicates use:

```c
if (sizegt("normaliser.cpp", 1)) return 0;
return 1;
```

The compiler reads `stx_size` and performs an unsigned byte-size comparison.

Permissive source-ingestion mode accepts symbolic return expressions from real
Coreutils files, such as `return EXIT_STATUS;`, and lowers them to status 0
with an explicit diagnostic. This proves that the macro-free frontend can
ingest the translation-unit shape, but it is not a semantic implementation of
the unresolved macro or helper function.

Lines beginning with `#` and `//` comments are skipped. The parsed return
status is lowered to Linux x86-64 `_start` assembler. Before emission, the
compiler runs an administrative typed term through the dependent NbE core;
the compiler now constructs a quoted beta-redex and applies the typed
`normalizeCode : Code(A) -> Code(A)` term before emitting assembly.

## Typed structs, pointers, calls, and recursion

`c_subset_semantics.cpp` provides the typed semantic core for the next source
layer. It represents the following C-like declarations without relying on a
host C compiler:

```c
struct Node { int value; struct Node *next; };
int length(struct Node *p) { return length(p); }
```

The checker has nominal struct types, named field declarations, `.` and `->`
field access, pointer formation and dereference, function signatures, arity
and argument checking, and recursive function declarations. A function is
entered in the function environment before its body is checked, so a
recursive call is well-scoped while an incorrect pointer result or unknown
field is rejected. Assignment expressions require an lvalue and matching
types; integer `+` and equality-shaped binary expressions require integer
operands. Conditional expressions require an integer condition and identical
branch types, allowing recursive bodies to express typed base cases and
recursive branches. `while` statements require an integer condition, check
their body, and have type `void`; sequences return the type of their final
item. Function references are first-class function types, and indirect calls
check the referenced signature before returning its result type. The
executable also sends a quoted beta-redex through the dependent NbE
`normalizeCode` term; its output is a regression test for this bridge.

This is the typed front-end boundary: general C function lowering and field
offset computation still need to be added before arbitrary GNU `find` code can
be assembled. The existing syscall subset remains usable for the small CLI
fixtures, and generated targets continue to use `as` and `ld` rather than
GCC.

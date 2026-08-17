# Macro-free C subset

The first source-ingestion slice is deliberately explicit. It accepts a
translation unit containing one function with the shape:

```c
int main(int argc, char **argv) { return DECIMAL; }
```

A small function-call form is also accepted when the translation unit defines
an identity helper:

```c
int identity(int value) { return value; }
int main(int argc, char **argv) { return identity(7); }
```

The frontend checks the helper shape and lowers this constant call result into
the generated exit status; an unknown callee is rejected. The assembler target
still uses `as` and `ld`.

Two-argument constant addition is also accepted:

```c
int add(int left, int right) { return left + right; }
int main(int argc, char **argv) { return add(2, 5); }
```

The call arity and helper name are checked, and the generated target exits with
the computed result.

Nullary constant helpers are supported as well:

```c
int answer() { return 9; }
int main(int argc, char **argv) { return answer(); }
```

The declared zero-argument signature is checked before assembly emission.

A minimal recursive declaration form is accepted and checked through the
typed function environment:

```c
int recurse(int n) { return recurse(n); }
int main(int argc, char **argv) { return recurse(0); }
```

`recursive_call.c` is the source-ingestion regression fixture. General
recursive execution and termination analysis remain semantic-core concerns;
this backend form only lowers the zero-call result.
The base-case form in `recursive_base_case.c` is checked through the
structured `if`/`return` checker and validates the decrementing recursive call.

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
String writes decode `\\n`, `\\t`, `\\r`, `\\a`, `\\b`, `\\f`, `\\v`, `\\?`, one- or two-digit hexadecimal and octal escapes, `\\0`, `\\\\`, and escaped quotes/apostrophes before length
checking and emission; `write_escape.c` and `write_high_byte.c` verify ordinary
and high-byte results.
Malformed hexadecimal escapes are rejected before assembly, covered by
`write_bad_hex_string.c`.
Out-of-range octal byte escapes are likewise rejected by
`write_bad_octal_string.c`.
The same malformed-hex rejection applies in counted loops, covered by
`loop_write_bad_hex.c`.
Out-of-range octal escapes in loop payloads are covered by
`loop_write_bad_octal.c`.
Malformed escapes in adjacent fragments are rejected by
`write_adjacent_bad_hex.c`.
Invalid hexadecimal digits in adjacent fragments are covered by
`write_adjacent_bad_hex_digits.c`.
Invalid octal digits in adjacent fragments are covered by
`write_adjacent_bad_octal_digits.c`.
Out-of-range octal fragments are rejected by
`write_adjacent_bad_octal.c`.
Unsupported escapes in adjacent fragments are rejected by
`write_adjacent_bad_escape.c`.
The loop equivalent is rejected by `loop_write_adjacent_bad_escape.c`.
`write_backslash.c` verifies literal backslash emission.
`write_quote.c` verifies an escaped quote in a string literal.
Consecutive literal writes are collected in source order after independent
escape and length checks; `write_two.c` and `write_three.c` verify the resulting
single syscall payload for multiple calls.
Adjacent string literals in one `write` argument are likewise concatenated
after decoding and length validation; `write_adjacent.c` and
`write_adjacent_three.c`, `write_adjacent_four.c`, and
`write_adjacent_five.c` cover two- through five-fragment forms.
`write_adjacent_nul.c` verifies NUL preservation across adjacent fragments.
`write_adjacent_high_byte.c` verifies high-byte preservation across fragments.
`write_adjacent_octal.c` verifies octal preservation across fragments.
`write_adjacent_control.c` verifies control-byte preservation across fragments.
`write_adjacent_whitespace.c` verifies newline and tab preservation across
fragments.
`write_adjacent_form_vertical.c` verifies form-feed and vertical-tab fragments.
`write_adjacent_binary_mixed.c` combines NUL and high-byte fragments.
`write_adjacent_mixed.c` combines question-mark and octal fragments.
`write_adjacent_empty.c` verifies empty adjacent fragments are accepted.
`write_adjacent_empty_trailing.c` covers a trailing empty fragment.
`write_adjacent_empty_both.c` covers empty leading and trailing fragments.
`write_adjacent_mixed_whitespace.c` combines newline and octal fragments.
`write_adjacent_mixed_tab_hex.c` combines tab and hexadecimal fragments.
`write_adjacent_mixed_cr_hex.c` combines carriage-return and hexadecimal
fragments.
`write_adjacent_mixed_form_hex.c` combines form-feed and hexadecimal fragments.
`write_adjacent_mixed_vertical_hex.c` combines vertical-tab and hexadecimal
fragments.
`write_adjacent_mixed_apostrophe_hex.c` combines apostrophe and hexadecimal
fragments.
`write_adjacent_mixed_backslash_hex.c` combines backslash and hexadecimal
fragments.
`write_adjacent_mixed_backslash_octal.c` combines backslash and octal
fragments.
`write_binary_four.c` covers four-fragment binary concatenation at top level.
`write_binary_three.c` covers the corresponding three-fragment form.
`write_binary_three_nonempty.c` adds an ordinary leading byte to the binary
three-fragment form.
`write_binary_four_nonempty.c` extends that coverage to four fragments.
`write_binary_four_full.c` places ordinary bytes around the binary fragments.
Counted-loop writes use the same decoding path, covered by
`loop_write_escape.c`.
`loop_write_hex.c` verifies that hexadecimal escapes use the same path.
`loop_write_high_byte.c` verifies repeated high-byte hexadecimal payloads.
`loop_write_octal.c` verifies repeated octal payloads.
`loop_write_hex_single.c` verifies repeated one-digit hexadecimal payloads.
`loop_write_quote.c` verifies escaped quotes in counted-loop payloads.
`loop_write_apostrophe.c` verifies escaped apostrophes in the same path.
`loop_write_question.c` verifies escaped question marks in counted loops.
`loop_write_carriage_return.c` verifies binary carriage-return bytes in loops.
`loop_write_control_escapes.c` verifies the remaining common control escapes.
`loop_write_nul.c` verifies embedded NUL bytes in a counted-loop payload.
`loop_write_adjacent_nul.c` verifies embedded NUL bytes across adjacent
literal fragments.
`loop_write_adjacent_high_byte.c` verifies high-byte values across fragments.
`loop_write_adjacent_newline.c` verifies newline bytes across fragments.
`loop_write_adjacent_tab.c` verifies tab bytes across fragments.
`loop_write_adjacent_carriage_return.c` verifies carriage-return bytes across
fragments.
`loop_write_adjacent_form_vertical.c` verifies form-feed and vertical-tab
fragments.
`loop_write_adjacent.c` verifies adjacent string fragments inside a counted
loop.
`loop_write_adjacent_hex.c` combines adjacent fragments with hexadecimal
escapes in that loop path.
`loop_write_adjacent_mixed.c` combines question-mark and hexadecimal escapes.
`loop_write_adjacent_mixed_octal.c` combines octal and tab escapes.
`loop_write_adjacent_binary_mixed.c` combines NUL and octal bytes across
adjacent fragments.
`loop_write_adjacent_octal.c` covers the corresponding octal form.
`loop_write_adjacent_control.c` covers adjacent control-byte fragments.
`loop_write_adjacent_three.c` extends that coverage to three fragments.
`loop_write_empty_fragments.c` verifies empty outer fragments in loops.
`loop_write_empty_nul.c` verifies an empty fragment before a NUL byte.
`loop_write_nul_empty_trailing.c` verifies an empty fragment after a NUL byte.
`loop_write_binary_three.c` combines empty, NUL, and high-byte fragments.
`loop_write_binary_three_nonempty.c` adds an ordinary leading byte to that
loop case.
`loop_write_binary_trailing_empty.c` adds a trailing empty binary fragment.
`loop_write_binary_four.c` adds a trailing empty fragment to that binary case.
`loop_write_binary_four_nonempty.c` adds ordinary leading bytes to the loop.
`loop_write_binary_four_full.c` places ordinary bytes around both binary
fragments in the loop.
`loop_write_binary_five_full.c` extends the full binary case to five fragments.
`loop_write_adjacent_four.c` extends it to four fragments.
`loop_write_adjacent_five.c` extends it to five fragments.

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

A bounded `while` form is also supported when its counter is initialized to
zero and compared against a literal:

```c
int i = 0;
while (i < 3) write(1, "w", 1);
```

It uses the same checked literal payload and counter lowering as the bounded
`for` form. Unbounded or non-literal `while` loops remain outside the subset.
Adjacent literal fragments, including embedded binary bytes, use the same
rules; `while_write_adjacent_binary.c` covers that case. Three adjacent while
fragments, including binary bytes, are covered by `while_write_three.c`.
Four adjacent while fragments are covered by `while_write_four.c`.
Five adjacent while fragments are covered by `while_write_five.c`, completing
the current while fragment-depth ladder.
The loop body may also be enclosed in braces, as in
`while_write_braced.c`.
Braced loops may include the explicit `i++` update, covered by
`while_write_explicit_increment.c`; the backend checks and supplies the same
single counter update.
The bound may be inclusive (`i <= N`), as tested by
`while_write_inclusive.c`; inclusive loops use the corresponding signed
comparison in the generated assembly.
Zero bounds are preserved as zero-iteration loops rather than being mistaken
for an absent loop; `while_write_zero.c` covers that edge case.
The inclusive counterpart `while_write_inclusive_zero.c` confirms that `i <= 0`
still executes once.
Inclusive while payloads may also use three adjacent fragments, covered by
`while_write_inclusive_three.c`.
Four-fragment inclusive while payloads are covered by
`while_write_inclusive_four.c`.
Five-fragment inclusive while payloads are covered by
`while_write_inclusive_five.c`.
The same zero-iteration guarantee applies to bounded `for` loops;
`loop_write_zero.c` covers that form.
Bounded `for` loops also accept inclusive bounds (`i <= N`), covered by
`loop_write_inclusive.c`.
Inclusive loops accept adjacent fragments and binary escapes as well;
`loop_write_inclusive_adjacent.c` covers that combination.
Three-fragment inclusive writes, including NUL and high-byte payloads, are
covered by `loop_write_inclusive_three.c`.
Four-fragment inclusive payloads are covered by
`loop_write_inclusive_four.c`.
Five-fragment inclusive payloads are covered by
`loop_write_inclusive_five.c`, completing the current fragment-depth ladder.
Inclusive payload lengths remain checked; malformed metadata is rejected by
`loop_write_inclusive_bad_length.c`.
Non-literal or otherwise unbounded `while` conditions are rejected explicitly;
`while_write_unbounded.c` covers that boundary.
The same rule applies to `for` bounds; `loop_write_unbounded.c` verifies that
non-literal bounds are rejected.
The reduced `for` form also requires `int i = 0`; nonzero initializers are
rejected by `loop_write_nonzero_init.c`.
Its update clause must be the literal `i++`; `loop_write_nonincrement.c`
verifies rejection of alternative updates.
Braced inclusive `for` bodies are covered by
`loop_write_inclusive_braced.c`.
This also composes with braced bodies and explicit `i++`, as shown by
`while_write_inclusive_braced.c`.

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

The source frontend also lowers a checked one-case switch on `argc`:

```c
switch (argc) { case 2: return 7; default: return 3; }
```

The selector and constant case label are checked through the semantic core;
the assembler emits a direct compare and two exit paths.
Both the matching-case and default runtime paths are covered by the switch
fixtures.
Two constant cases are also lowered in source order, with duplicate labels
rejected by the typed switch checker.
Two-value `enum` declarations can provide named integer case labels; their
values are checked for collisions before lowering.
Implicit two-value declarations (`First, Second`) receive the standard
zero-based values before semantic validation and lowering.
`enum_switch_duplicate.c` verifies that colliding named values are rejected.
`enum_switch_duplicate_name.c` verifies duplicate enumerator names are also
rejected by the shared validator.
The semantic core represents enums nominally, gives them four-byte layout,
and treats them as integer-like switch selectors while preserving enum-name
equality.
Enum types also participate in the existing alias environment, including
duplicate-alias rejection.
The semantic validator rejects unnamed enum declarations so nominal equality
cannot collapse distinct anonymous cases.
Declaration validation also rejects duplicate enumerator names and duplicate
numeric values.
The source enum-switch path invokes this same validator before lowering.
The semantic core also has a distinct `char` type with one-byte layout; it is
not definitionally equal to `int`, and `allocate(char)` yields `char *`.
The source frontend lowers `sizeof(char)` to 1.
Byte-sized char fields compose in packed aggregates; a following `int` field
starts at offset 1 in the current layout model.
Char-pointer addition preserves `char *`, and indexing a char pointer returns
the distinct `char` type.
Char types are also preserved through function parameters and return values;
integer arguments are rejected for char-typed parameters.
Char-valued callback pointers preserve the same signature through indirect
calls.
Character pointers retain the ordinary eight-byte pointer layout;
`sizeof(char *)` lowers to 8.
Single-byte character literals such as `'A'` lower to their unsigned byte
value.
Common escapes `\n`, `\t`, `\\`, and `\'` are decoded in character literals.
Each escape has an individual assembler regression fixture.
Two-digit hexadecimal escapes such as `\x41` are also decoded.
Three-digit octal escapes such as `\101` are decoded as well.
Malformed hexadecimal and octal character escapes are rejected.
Multi-byte character literals are rejected as well.
Constant bitwise and shift return expressions are lowered to their computed
integer exit statuses; `bitwise_return.c` and `shift_return.c` cover these
assembler paths.
Enum pointers retain the ordinary eight-byte pointer layout, and
`allocate(enum Mode)` is checked as `enum Mode *`.
Enums may be stored in nominal struct fields; field typing, packed offsets,
and aggregate size preserve the enum type.
Assignments through `.` and `->` require a matching enum type; plain integer
expressions are rejected for nominal enum fields.
Indexing an `enum Mode *` preserves `enum Mode` as the element type, while
indexing a scalar enum is rejected.
Enum types are preserved in function parameters and return types; calls with
plain integer arguments are rejected.
The same signatures survive function-address formation and indirect callback
calls, including enum-valued results.
The semantic regression directly checks an enum-typed selector through
`check_switch`, in addition to the source-level named-label fixture.
`sizeof(enum Mode)` uses the typed four-byte layout and undeclared enum names
are rejected.

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
check the referenced signature before returning its result type. Function
signatures may also be named through typedef aliases; resolving a callback
alias preserves its argument and result types for later calls. Pointer aliases
to callback signatures are preserved as well, covering `typedef`-style
callback pointer declarations. Such a pointer can be dereferenced and then
used as an indirect-call callee, with the callback signature checked after
dereference. Pointer-to-function and nested callback-pointer types retain the
ordinary eight-byte pointer size and deterministic field offsets. Nested pointer layers are handled compositionally, so a
pointer-to-pointer callback can be dereferenced twice before an indirect call.
Taking the address of a function reference produces a
pointer-to-function value; dereferencing that address restores the signature
for an indirect call. Function pointers can be compared with other compatible
function pointers; comparisons against integers or unrelated types are
rejected. Function-typed
Struct fields declared as pointer-to-function values can receive `&function`
and dispatch after `->` plus dereference, preserving the same checked
signature.
Function-typed globals can be initialized by matching function references and
are checked through the same signature path before lowering. Pointer-to-function
globals may likewise be initialized from a function address only when the
declared pointer signature matches. Higher-order functions may
An integer or otherwise incompatible initializer for such a global is rejected
before lowering.
take such callbacks as parameters; an `apply_callback`-shaped function can
invoke its callback only with arguments matching that signature. The
same checking also permits function-valued returns, so a factory returning a
function reference must return the exact declared callback signature. The
the returned function can itself be passed to an indirect call, so nested
higher-order composition is checked without erasing its signature.
Struct fields may themselves carry function signatures; member selection
preserves that signature, allowing a checked indirect call through a callback
field.
The same preservation holds through `->` on a pointer to that struct, so
callback dispatch remains typed after pointer member selection.
Function-pointer fields use the core’s eight-byte pointer layout; the recursive
callback-object fixture checks both its callback offset and its self-pointer
offset before any lowering occurs.
Callback fields are assignable lvalues as well: both `.` and `->` assignments
require an exactly matching function signature, rejecting integer or unrelated
function values.
Recursive callback chains such as `node->next->run(x)` preserve the same
signature through each pointer traversal before dispatch.
Callback signatures may return `void`; the checker validates void-returning
indirect calls and higher-order consumers without treating them as integers.
Recursive functions may carry callback parameters; recursive re-entry and
indirect callback calls are checked against the same preserved signature.
Mutually recursive functions can likewise pass callback parameters between
each other while retaining the signature at every indirect call.
Malformed mutual callback calls, including wrong callback argument types, are
rejected by whole-program checking.
index expression `p[i]` requires a pointer base and integer index, while
integer-plus-pointer arithmetic preserves the pointer element type. The
operator vocabulary also includes subtraction, ordering, and logical forms;
the current expression checker represents their result as `int`.
The typed operator vocabulary additionally includes bitwise `&`, `|`, `^`
and shifts `<<`, `>>`, all requiring integer operands and producing `int`.
Integer remainder `%` is also typed and constant-lowered.
Constant modulo by zero is rejected before host evaluation.
Integer division `/` is typed and constant-lowered; a constant zero divisor
is rejected before assembly.
The comparison operators `>`, `<=`, `>=`, and `!=` are also typed and
constant-lowered as integer Boolean results.
Constant logical `&&` and `||` expressions use C truthiness and lower to 0 or
1 exit statuses.
Constant ternary expressions `condition ? yes : no` select and lower the
corresponding integer branch.
Constant unary `!` and `-` expressions are lowered as logical-not and
arithmetic-negation results.
Constant integer addition and subtraction expressions are lowered directly to
their computed exit statuses.
Constant integer multiplication is lowered the same way.
typed `sizeof` form returns an integer and uses the core's deterministic
packed layout: `int` is 4 bytes, pointers and function references are 8 bytes,
and struct size is the sum of its field sizes. This is a deliberate initial
layout model; ABI alignment and unions remain future extensions.
Field order is retained explicitly, so `Node.value` has offset 0 and
`Node.next` follows it at offset 4 in the example layout. The semantic core
exposes this checked offset information for future `.` and `->` lowering.
`validate_structs` rejects duplicate field names before whole-program function
checking, preserving unambiguous nominal field lookup.
`validate_function` likewise rejects duplicate parameter names before a
function environment is created.
`validate_struct_cycles` rejects by-value recursive structs, while pointer
recursive fields remain valid and are the representation used by `Node.next`.
The `Aliases` environment provides transitive `typedef` resolution and rejects
alias cycles before a resolved type is consumed by the checker.
`Global` declarations form an ordered environment: duplicate names are
rejected and each initializer is checked against its declared type, with
earlier globals available to later initializers.
Typed allocation `allocate(T)` produces `T*` after checking that `T` has a
known size; `release(p)` accepts only pointer expressions and has type `void`.
These operations currently describe the checked semantic boundary; lowering
them to Linux allocation syscalls is a separate backend step. The executable
also sends a quoted beta-redex through the dependent NbE
`normalizeCode` term; its output is a regression test for this bridge.

The semantic core also exposes a statement checker for `return`, expression
statements, `if`, and `while`. It checks return expressions against the
function result type and checks nested branches and loop bodies. This permits
recursive functions to be represented with explicit C-like control-flow
bodies; the permissive source reader and assembler lowering have not yet been
replaced by this structured frontend. `break` and `continue` are checked with
an explicit loop-depth context and are rejected outside loop bodies. `for`
statements check their initializer, integer condition, update expression, and
body under the same loop context.
`do...while` checks its body first and then requires an integer post-condition,
also preserving loop scope for `break` and `continue`.
The semantic API additionally checks `switch` selectors and case labels as
integers and type-checks every case and default body. Switch cases are exposed
as a structured checker API, requires constant labels, and rejects duplicate
labels; parser integration remains future work.
Definite-return analysis rejects fall-through in non-void structured
functions; an `if` is considered complete only when both branches return.
Loops are conservatively treated as potentially terminating.

Defining `CSEM_LIBRARY` when compiling `c_subset_semantics.cpp` omits the
regression-demo `main` and exposes the typed core for reuse by a parser or
assembler backend. The default build retains the executable test harness.
`check_program` constructs the complete function environment before checking
bodies, so mutually recursive declarations are accepted and duplicate names
are rejected. The compiler's pre-emission gate exercises this mutual-recursion
path.
The source reader recognizes the recursive-node declaration shape used by the
typed core and rejects the corresponding by-value self-cycle before assembly.
`fixtures/struct_pointer.c` is accepted and `fixtures/struct_value_cycle.c` is
rejected.
The constant struct-field form in `fixtures/struct_field_return.c` lowers
`node.value` to the initialized exit status, providing the first source-level
field-read path.
The corresponding address/arrow form `(&node)->value` is supported by
`fixtures/struct_arrow_return.c`, which exits with the initialized value.
Field assignment forms are supported for both `.` and `->`; the fixtures
`struct_field_assign.c` and `struct_arrow_assign.c` verify exit statuses `6`
and `11` respectively.
`struct_next_return.c` additionally assigns `node.next = &node` and reads
`node.next->value`, verifying self-referential pointer fields with status `12`.
Constant three-element integer arrays can be indexed in the source subset;
`array_index_return.c` returns `values[1]` with status `13`, while an out of
bounds constant index is rejected.
Indexed assignment is also supported; `array_assign_return.c` writes
`values[1] = 14` and returns status `14`.
Array-to-pointer initialization is supported for the fixed array form;
`pointer_array_return.c` initializes `int *p = values` and returns `p[1]` with
status `15`.
Pointer-indexed assignment is supported as well; `pointer_array_assign.c`
writes `p[1] = 16` and returns status `16`.
Pointer addition and dereference are supported in the fixed form
`int *q = p + 1; return *q;`; `pointer_add_deref.c` returns status `17`.
The fixed-array `sizeof` form uses the packed layout size and
`array_sizeof_return.c` returns status `12` for three four-byte integers.
`sizeof(struct Node)` is also lowered from the typed struct layout;
`struct_sizeof_return.c` returns status `12`.
`offsetof(struct Node, next)` consumes the checked field offset and returns
status `4` in `node_offsetof_return.c`.
`sizeof(int)` is lowered using the scalar layout size; `int_sizeof_return.c`
returns status `4`.
`sizeof(int *)` uses the typed pointer size and `pointer_sizeof_return.c`
returns status `8`.
The null-pointer guard form validates pointer equality and lowers
`if (p == 0)` for a zero-initialized `int *`; `null_guard.c` exits with status
`1`.
Simple file-scope integer declarations with constant initializers are also
validated through `check_globals`; duplicate names are rejected. The current
assembler slice resolves a return of such a global and emits its constant
value. `fixtures/global_return.c` exits with the initialized value.
The source slice also recognizes a small C-style function-pointer form:
`fixtures/function_pointer.c` assigns `identity` to `fp`, checks the typed
address/dereference/indirect-call path, and exits with status `6`.
`function_pointer_bad_arity.c` is rejected when the indirect call supplies two
arguments to the one-argument pointer.
`function_pointer_global.c` covers the corresponding file-scope callback
pointer initializer.
`function_pointer_global_address.c` covers the explicit `&identity` spelling
for the file-scope form.
`function_pointer_global_explicit_deref.c` covers `(*fp)(6)` for that form.
`function_pointer_global_binary.c` covers a two-argument file-scope callback.
`function_pointer_global_binary_direct.c` covers its direct `fp(2, 5)` spelling.
`function_pointer_global_nullary.c` covers a zero-argument file-scope callback.
`function_pointer_global_nullary_direct.c` covers its direct `fp()` spelling.
`function_pointer_global_binary_bad_arity.c` is rejected when the binary global
callback is invoked with only one argument.
The direct-call variant `function_pointer_global_binary_direct_bad_arity.c`
is rejected as well.
`function_pointer_global_nullary_bad_arity.c` rejects an argument supplied to
a zero-argument global callback.
The local equivalent `function_pointer_binary_bad_arity.c` is rejected by the
same declaration-aware check.
`function_pointer_nullary.c` covers an empty-argument callback signature and
the corresponding `fp()` call.
`function_pointer_nullary_explicit_deref.c` covers the explicit `(*fp)()` form.
`function_pointer_binary.c` covers a two-argument callback and indirect call.
`function_pointer_binary_explicit_deref.c` covers the corresponding
`(*fp)(2, 5)` spelling.
`function_pointer_alias.c` checks assignment from one compatible callback
pointer variable to another before indirect dispatch.
`function_pointer_binary_alias.c` applies the same aliasing path to a binary
callback signature.
`function_pointer_binary_alias_explicit_deref.c` covers `(*gp)(2, 5)` after
the compatible alias assignment.
`function_pointer_nullary_alias.c` covers compatible aliasing for an empty
argument callback.
`function_pointer_nested.c` covers a pointer-to-callback pointer initialized
from `&fp` and dispatched through two explicit dereferences.
`function_pointer_nested_unary.c` applies the same two-level traversal to an
`int -> int` callback and checks that its argument signature is preserved.
`function_pointer_nested_binary.c` extends the traversal to a two-argument
callback and returns the checked sum.
`function_pointer_parameter.c` checks a higher-order function whose typed
callback parameter is invoked inside its body.
`function_pointer_binary_parameter.c` applies the same higher-order check to
a callback with two integer arguments.
`function_pointer_typedef.c` checks a source-level typedef alias for a unary
callback pointer before indirect dispatch.
`function_pointer_binary_typedef.c` checks the corresponding two-argument
typedef callback alias.
`function_pointer_nullary_typedef.c` checks the zero-argument typedef callback
alias and its indirect call.
`function_pointer_typedef_parameter.c` passes a typedef callback alias as a
typed higher-order parameter and invokes it in the callee.
`function_pointer_binary_typedef_parameter.c` extends that higher-order alias
path to a two-argument callback.
`function_pointer_void.c` checks a void-returning callback pointer invocation
before the integer `main` result is returned.
`function_pointer_void_binary.c` checks the same void-returning path with two
integer callback arguments.
`function_pointer_void_typedef.c` checks a named typedef alias for a void
callback signature.
`function_pointer_void_binary_typedef.c` extends the named void alias to two
integer arguments.
`function_pointer_void_parameter.c` checks a void higher-order function that
receives and invokes a void callback parameter.
`function_pointer_void_binary_parameter.c` extends that parameter path to a
void callback with two integer arguments.
`function_pointer_equality.c` checks equality of two compatible function
pointers after typed address formation.
`function_pointer_struct_field.c` stores a typed callback in a struct field
and dispatches through the checked member expression.
`function_pointer_struct_binary_field.c` applies the same member dispatch to
a two-argument callback field.
`function_pointer_struct_void_field.c` checks a void callback stored in a
struct field and invoked through the member.
`function_pointer_struct_nullary_field.c` checks a zero-argument integer
callback stored and dispatched through a struct member.
`function_pointer_struct_typedef_field.c` uses a typedef callback alias as the
struct field type before member dispatch.
`function_pointer_struct_arrow_field.c` stores and dispatches a callback
through a pointer-to-struct `->` member expression.
`function_pointer_struct_arrow_binary_field.c` extends arrow-member dispatch
to a two-argument callback.
`function_pointer_struct_arrow_void_field.c` checks a void callback through a
pointer-to-struct arrow member.
`function_pointer_struct_arrow_nullary_field.c` checks a zero-argument integer
callback through the same arrow-member path.
`function_pointer_struct_aggregate.c` checks aggregate initialization of a
callback field before direct member dispatch.
`function_pointer_struct_binary_aggregate.c` extends aggregate callback
initialization to a two-argument field.
`function_pointer_struct_nullary_aggregate.c` covers aggregate initialization
of a zero-argument callback field.
`function_pointer_struct_void_aggregate.c` covers aggregate initialization of
a void callback field.
`function_pointer_struct_typedef_aggregate.c` combines aggregate initialization
with a typedef-named callback field.
`function_pointer_struct_binary_typedef_aggregate.c` extends that alias-aware
aggregate path to a two-argument callback.
`function_pointer_struct_void_typedef_aggregate.c` applies the alias-aware
aggregate path to a void callback field.
`function_pointer_struct_void_binary_typedef_aggregate.c` extends it to a
two-argument void callback field.
`function_pointer_struct_typedef_arrow.c` combines a typedef callback field
with pointer-to-struct arrow dispatch.
`function_pointer_struct_binary_typedef_arrow.c` extends that alias-aware
arrow dispatch to a two-argument callback.
`function_pointer_struct_void_typedef_arrow.c` extends alias-aware arrow
dispatch to a void callback.
`function_pointer_struct_multiple_fields.c` checks independent callback fields
and dispatches through the second member.
`function_pointer_struct_mixed_fields.c` checks a record containing both an
integer-returning callback and a void callback.
`function_pointer_struct_mixed_binary_fields.c` applies the mixed-signature
record check to two-argument callbacks.
`function_pointer_struct_mixed_aggregate.c` checks aggregate initialization of
both returning and void callback fields.
`function_pointer_struct_mixed_binary_aggregate.c` extends that mixed
aggregate path to two-argument callbacks.
`function_pointer_struct_nested_field.c` checks a pointer-to-callback pointer
stored in a struct field and dispatched through two dereferences.
`function_pointer_alias_bad_type.c` is rejected when the source and destination
callback signatures differ.
The semantic core applies the same alias-assignment rule to two-argument
callback signatures before binary indirect dispatch.
`function_pointer_address.c` accepts the explicit `&identity` spelling for a
function-pointer initializer.
`function_pointer_explicit_deref.c` accepts the explicit `(*fp)(6)` call
spelling as well.
The two-argument `(*fp)(1, 2)` variant is rejected by the negative fixture
`function_pointer_explicit_deref_bad_arity.c`.
An integer file-scope initializer for a function pointer is rejected explicitly;
see `function_pointer_global_bad_init.c`.
For the recognized recursive-node declaration, file-scope pointer globals such
as `struct Node *head;` are checked as `Node*`; see
`fixtures/pointer_global.c`.
Duplicate pointer globals are rejected as well; see
`fixtures/duplicate_pointer_global.c`.

This is the typed front-end boundary: general C function lowering and field
offset computation still need to be added before arbitrary GNU `find` code can
be assembled. The existing syscall subset remains usable for the small CLI
fixtures, and generated targets continue to use `as` and `ld` rather than
GCC.

`c_subset_compiler.cpp` now includes the reusable core and runs a typed
recursive `struct`/pointer function check during its pre-emission validation,
alongside the dependent NbE check. Thus every emitted assembler target passes
both staging and the typed C semantic gate, although the permissive source
reader still lowers only its documented syscall subset.

## Coq semantic reference and regression suite

`semantics.v` records classic relational big-step judgments for the dependent
core (`eval`) and C subset (`ceval`), together with small-step relations
(`red` and `cstep`). It also names the intended preservation and normalizer
soundness theorems. The admitted proof bodies are an explicit proof-development
boundary; the C++ implementation remains the executable artifact.

Run `./test_c_subset_assembler.sh` to build the host compiler, emit assembly,
assemble and link with only `as --64` and `ld`, then run positive fixtures and
check rejection fixtures.

`semantics.ott` is the corresponding Ott source. It defines the shared syntax,
typing judgments, reduction rules, NbE-oriented big-step evaluation, and the
C-subset big-step/small-step judgments. Ott can generate Coq and LaTeX from
this source; building Ott from its upstream repository currently requires
`ocamlfind`, which is not installed on this server.
When Ott is installed, generate its output reproducibly with
`./generate_semantics.sh tex [OUTPUT]` or
`./generate_semantics.sh coq [OUTPUT]`; `./generate_semantics.sh check`
performs a syntax/type-checking run. The `OTT_BIN` environment variable can
select a non-default Ott executable.

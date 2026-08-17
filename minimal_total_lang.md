# Minimal total ADT language experiment

This branch isolates the smallest useful kernel for the normalizer project.
`minimal_total_lang.v` uses intrinsically typed inductive syntax:

- `Nat0` and `Bool0` are inductive data types;
- `Tm0 A` is an indexed expression type, so ill-typed addition and branches
  cannot be constructed;
- `eval0` is a structurally recursive total evaluator;
- `normalise0` evaluates and re-quotes a term;
- `Code0 A` makes staging explicit without executing quoted syntax;
- Coq proves quotation, idempotence, staging, and totality lemmas.

This is the verified first milestone. A typed de Bruijn/`let` layer was
prototyped on this branch but is intentionally not part of the checked kernel:
its environment lookup requires an explicit dependent equality proof. That is
the next proof obligation, not an unverified claim of completion.

The experiment deliberately omits variables, functions, effects, pointers, and
general recursion. That omission is the point: it gives a small trusted core
whose invariants can be transported to progressively larger languages.

## Route toward C

1. Add typed lambdas/application over the same indexed environment.
2. Add finite products and tagged unions for C structs and enums.
3. Add a separate typed memory model for pointers and arrays.
4. Add statements and a total, fuel-indexed expression normalizer.
5. Encode a deliberately bounded C fragment and compare its generated assembly
   traces with the existing compiler.

General C remains outside this experiment until each extension has a total
semantics and a checked preservation theorem. In particular, unrestricted
recursion and system calls belong at an explicit effect boundary rather than in
the pure normalizer.

## Verification

```sh
coqc minimal_total_lang.v
```

The CI workflow compiles this file alongside the existing core and C semantics.

## ARM64 compiler

`minimal_arm64_compiler.cpp` is the first executable backend for this language.
It accepts the constructors `nat`, `true`, `false`, `add`, and `if` in a small
S-expression syntax. It normalizes the ADT term before emitting AArch64 Linux
assembly. The generated `_start` places the normalized natural/boolean result
in `x0` and exits through syscall 93. `test_minimal_arm64.sh` checks the emitted
instruction contract and assembles the result with `aarch64-linux-gnu-as` when
that cross-toolchain is installed.

The lowering path is staged: it quotes the parsed term as `Code`, applies the
normalizer to that code value, and unquotes only the normalized result. Thus
the compiler uses the same explicit code boundary as the formal kernel rather
than having a separate ad-hoc constant-folding phase. A future compiler pass
can be represented by the same `Code` interface and subjected to the same
normalization step.

```sh
printf '%s\n' '(add (nat 2) (if true (nat 3) (nat 4)))' \
  | ./minimal_arm64_compiler-ci
```

This backend intentionally compiles only closed, pure ADT terms. Variables,
functions, memory, and effects remain future extensions with separate typing
and lowering proofs.

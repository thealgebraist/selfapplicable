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

The experiment deliberately omits variables, functions, effects, pointers, and
general recursion. That omission is the point: it gives a small trusted core
whose invariants can be transported to progressively larger languages.

## Route toward C

1. Add de Bruijn variables and an environment indexed by `Ty0`.
2. Add a total `let`/function fragment and prove substitution and preservation.
3. Add finite products and tagged unions for C structs and enums.
4. Add a separate typed memory model for pointers and arrays.
5. Add statements and a total, fuel-indexed expression normalizer.
6. Encode a deliberately bounded C fragment and compare its generated assembly
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

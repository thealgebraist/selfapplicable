# Two-stage total ADT language

This experiment separates a program into two explicit stages:

```text
type stage : Checker2 -> Tm2 -> CheckResult2
runtime stage : Tm2 -> Val2
```

`Checker2` is itself an inductive program. A user supplies a checker value,
and the kernel runs it with `run_type_stage2`. There is no hidden universal
type checker for the language: the user's type stage decides which constructors
and combinations it accepts.

## Minimal syntax

The runtime syntax has natural numbers, booleans, addition, conditionals,
pairs, and projections. The type-stage syntax has checker constructors such as
`CNat2`, `CBool2`, `CAdd2`, `CIf2`, and `CPair2`. A checker is finite data, so
running it is structurally recursive and total.

For example, this checker accepts exactly an addition of two natural literals:

```text
arithmetic_stage2 = CAdd2 CNat2 CNat2
```

The same runtime term may be accepted or rejected by different type stages.
That is the intended customization point.

## What the kernel guarantees

The kernel guarantees that a supplied type-stage program terminates and returns
either `Accepted2 ty` or `Rejected2`. It then runs the runtime stage only after
acceptance. It does **not** prove that a user-written checker is a sound model
of some intended type theory. If a user writes a checker that accepts unsafe
programs, the checker is wrong by specification, not by a failure of totality.

This is a rigorous language architecture, but it shifts trust: the checker
program and its specification become part of the trusted input. A later layer
can prove a particular checker sound with a preservation theorem.

## Files

- `two_stage_adt.v`: Coq definitions, totality lemma, checker examples, and a
  staged runtime theorem by computation.
- `minimal_arm64_compiler.cpp`: executable staged compiler used by the ARM64
  experiment.

The next extension is to add a typed environment and `let` to `Tm2`, then
define the corresponding checker constructors in `Checker2`. Functions and
general recursion should remain explicit later stages with separate totality
arguments.

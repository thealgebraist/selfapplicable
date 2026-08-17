# Self-applicable dependently typed normaliser

This directory contains a compact C++23 prototype of the core described in
`normaliser.tex`.

Build and run:

```sh
g++ -std=c++23 -Wall -Wextra -pedantic -O2 normaliser.cpp -o normaliser
./normaliser
```

The executable exercises:

- de Bruijn variables, sorts, Pi types, lambdas, applications, quotation, and unquotation;
- capture-avoiding shifting and substitution;
- bidirectional type checking for Pi/lambda terms;
- predicative universe checking (`Type i : Type (i+1)` and Pi level maxima);
- normalization by evaluation with semantic closures, neutrals, reflection, and type-directed eta-long reification;
- an explicit `Code` quotation boundary and staged self-application test.
- typed `Code(A)` terms: `quote(t) : Code(type(t))` and `unquote(c) : A` when `c : Code(A)`.

The implementation is intentionally small and uses substitution only for
dependent codomain instantiation. Quoted syntax is inert, and the staged entry
point executes a quoted normalizer only through the explicit `unquote_code`
boundary. The executable also checks eta-long reification of an open neutral
function.

Semantic sources and checks

`semantics.v` contains the Coq relations for the dependent core and the
small C expression bridge. `semantics.ott` is the corresponding Ott source.
Run `./check_semantics.sh` when `coqc` and/or `ott` are installed; each
available checker is run independently and unavailable tools are reported as
skipped.

Create a reproducible source bundle with:

```sh
./make_release_bundle.sh
```

The bundle includes `MANIFEST.sha256`; the archive itself gets a companion
SHA-256 file. Target validation remains assembler-only (`as --64` followed by
`ld`).

# Self-applicable dependently typed normaliser

This directory contains a compact C++23 prototype of the core described in
`normaliser.tex`.

Build and run:

```sh
make check-normaliser
```

Run the complete local checks with `make check`; create a release bundle with
`make release`.

ADT `find` CLI
--------------

`minimal_find_cli.cpp` is a typed-ADT filesystem query tool. It constructs a
finite predicate tree, checks that tree before traversal, and then evaluates it
against directory entries:

```text
find_adt ROOT [-name|-iname GLOB] [-path GLOB] [-type f|d|l]
             [-size [+|-]BYTESc] [-empty] [-prune] [-not] [-o] [-print0]
             [-maxdepth N] [-mindepth N]
```

Run `make check-find` for its regression suite. This is an explicit total
predicate language rather than complete GNU `find` compatibility; actions such
as `-exec` and `-delete` remain later extensions.

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

Minimal functional scope

The minimal usable deliverable is the dependent NbE core together with the
assembler-only x86-64 C-subset path. It is exercised by the complete
`test_c_subset_assembler.sh` fixture suite and the CI workflow. Full formal
completion still requires replacing the two admitted Coq theorems in
`semantics.v` and checking generated artifacts from `semantics.ott`; a general
C parser and complete shell-command compatibility are broader follow-on
projects.

Verify both layers with:

```sh
./verify_release_bundle.sh dist/selfapplicable-VERSION.tar.gz
```

Live work-matrix publication

`WORK_MATRIX.md` is served directly from the working tree through Cloudflared,
and is also rendered as HTML, so the public matrix reflects edits on the next
request. The current endpoints are:

`https://brokers-serving-pairs-duck.trycloudflare.com/WORK_MATRIX.md`

`https://brokers-serving-pairs-duck.trycloudflare.com/WORK_MATRIX.html`

Run `./publish_matrix_cloudflared.sh` to start the same arrangement elsewhere.

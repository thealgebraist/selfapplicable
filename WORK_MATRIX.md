# Self-Applicable C-Subset Work Matrix

Legend: `[x]` verified in the current repository; `[ ]` still to do.

| Domain | Task | Status | Evidence / next step |
|---|---|---:|---|
| Core language | Dependent syntax and NbE normalizer | [x] | `normaliser.cpp`, dependent NbE tests |
| Core language | Predicative universes and staged quotation | [x] | `normaliser.cpp`, staged examples |
| Core language | Bidirectional checking and conversion | [x] | `normaliser.cpp` and checker tests |
| Core language | Inductive/ADT-oriented integer examples | [x] | Existing dependent-core examples |
| Core language | Coq staged normalizer specification compiles | [x] | CI run `32038168585` compiles `normaliser_core.v` |
| Core language | Coq staged quotation typing lemmas | [x] | CI run `32038300860` proves quote/unquote typing lemmas |
| Core language | Coq typed application lemma | [x] | CI run `32038978602` proves typed function application |
| Core language | Coq staged reduction round-trip | [x] | CI run `32039417369` compiles `nred`, `staged_round_trip`, and the typed round-trip theorem |
| Core language | Coq reflexive-transitive staged reduction | [x] | CI run `32039514912` compiles `nred_star` and `staged_round_trip_star` |
| Core language | Typed preservation for staged reduction | [x] | CI run `32039783247` proves preservation for the explicit `nstage` quote/unquote step |
| Core language | Transitivity of staged reduction closure | [x] | CI run `32039878989` proves `nred_star_trans` |
| Core language | Staging/reduction interface lemmas | [x] | CI run `32039960975` proves `nred_star_refl` and `nstage_is_nred` |
| Core language | Lift staging steps into reduction closure | [x] | CI run `32040043748` proves `nstage_to_nred_star` |
| Core language | Compose staging with later reductions | [x] | CI run `32040215194` proves `nstage_then_nred_star` |
| Core language | Lift ordinary steps into reduction closure | [x] | CI run `32040318557` proves `nred_to_nred_star` |
| Core language | Explicit typed staged-normalization result contract | [x] | CI run `32040430580` compiles `staged_normalise_result` and its typing/reachability projections |
| Core language | Closure compatibility under quote/unquote | [x] | CI run `32040514525` proves `nred_star_quote` and `nred_star_unquote` |
| Core language | Closure compatibility under application | [x] | CI run `32040605802` proves `nred_star_app_left` and `nred_star_app_right` |
| Core language | Closure compatibility under lambda binders | [x] | CI run `32040705678` proves `NRLamBody` and `nred_star_lam` |
| Core language | De Bruijn beta substitution in Coq reduction | [x] | CI run `32040793617` compiles `nshift`, `nsubst0`, and substitution-producing `NRBeta` |
| Core language | Coq beta substitution sanity lemmas | [x] | CI run `32040998545` proves `nsubst0_var0`, `nsubst0_var_succ`, and `beta_identity` |
| Core language | Outer-variable beta substitution witness | [x] | CI run `32041092698` proves `beta_outer_variable` with binder index adjustment |
| Core language | Binder-sensitive substitution equations | [x] | CI run `32042163802` proves lambda-bound and lambda-outer substitution equations |
| Core language | Structural substitution equations | [x] | CI run `32042396965` proves application, quote, and unquote substitution lemmas |
| Core language | Beta witnesses in multi-step closure | [x] | CI run `32042515552` proves `beta_identity_star` and `beta_outer_variable_star` |
| Core language | Combined application closure compatibility | [x] | CI run `32042623643` proves `nred_star_app_both` |
| Core language | Lambda-application closure sequencing | [x] | CI run `32042712233` proves `nred_star_lam_app` |
| Core language | Staged execution followed by normalization | [x] | CI run `32042807905` proves `nred_star_unquote_quote_then` |
| Core language | General one-hole reduction contexts | [x] | CI run `32042898162` proves `nctx`, `nplug`, and `nred_star_plug` |
| Core language | Composable reduction contexts | [x] | CI run `32042983327` proves `nctx_compose` and `nplug_compose` |
| Core language | Context composition associativity | [x] | CI run `32043082798` proves `nctx_compose_assoc` |
| Core language | Context composition identity laws | [x] | CI run `32043169892` proves hole-left and hole-right identity lemmas |
| Core language | Nested context reduction bridge | [x] | CI run `32043338028` proves `nred_star_nested_plug` |
| Core language | Binder-free context substitution bridge | [x] | CI run `32043851603` proves `nctx_flat`, `nctx_subst_flat`, and `nsubst_flat_plug` |
| Core language | Flat-context composition closure | [x] | `nctx_flat_compose` verified by CI run `32044236173` |
| C subset | Integer returns and conditionals | [x] | `fixtures/` regression suite |
| C subset | Structs and field access | [x] | `struct_*` fixtures |
| C subset | Pointers and pointer arithmetic | [x] | `pointer_*` fixtures |
| C subset | Function calls and function pointers | [x] | `function_*` fixtures |
| C subset | Recursion with bounded/base cases | [x] | `recursive_*` fixtures |
| C subset | Arrays, `sizeof`, and `offsetof` | [x] | `array_*`, `*_sizeof_*`, `*_offsetof_*` fixtures |
| C subset | Enums and switch dispatch | [x] | One- through five-case switch fixtures |
| C subset | Counted `break` | [x] | `fixtures/loop_break.c` |
| C subset | Counted `continue` | [x] | `fixtures/loop_continue.c` |
| C subset | `for`/`while` counted loops | [x] | Loop fixtures and emitter |
| C subset | Bash-like filesystem commands | [x] | `ls`, `find`, `pwd`, `echo`, and related fixtures |
| Target | x86-64 assembly emission | [x] | Compiler emits `.s` directly |
| Target | Assembly-only build path | [x] | Tests use `as --64` and `ld`; no GCC target compilation |
| Target | Syscall-based runtime primitives | [x] | Large syscall/query fixture matrix |
| Target | Safe syscall probes for legacy APIs | [x] | `afs`, `nfsservctl`, `vserver`, `ustat`, thread-area probes |
| Target | Safe syscall probes for modern APIs | [x] | `fchmodat2`, `mmap`, `pidfd`, `landlock`, `futex`, and others |
| Target | `nice(0)` lowering | [x] | Corrected away from blocking syscall 34 |
| Target | Nonzero `nice(n)` lowering | [x] | `getpriority`/`setpriority` sequence and `nice_one.c` |
| Target | Output write register correctness | [x] | Fixed 32/64-bit `writefd` register mismatch |
| Testing | Per-target timeout in status tests | [x] | `test_c_subset_assembler.sh` |
| Testing | Timeout coverage for output/reject helpers | [x] | `test_c_subset_assembler.sh` |
| Testing | Continuous assembler-only CI gate | [x] | GitHub Actions run `32036698013` passed |
| Testing | Full regression run under current host load | [x] | Local `./test_c_subset_assembler.sh` passes; deterministic CI gates pass in run `32052301242`, while capability probes vary by runner |
| Semantics | Coq big-step semantics | [ ] | `small_step_preserves_big_step` and `normaliser_sound` remain admitted |
| Semantics | Coq small-step semantics | [ ] | Add preservation/progress proofs for the C subset |
| Semantics | Coq semantic source compiles | [x] | CI run `32037863355` installs Coq and compiles `semantics.v` |
| Semantics | C store update lemmas | [x] | CI run `32038550737` proves same-slot and distinct-slot update properties |
| Semantics | Typed store assignment preservation | [x] | CI run `32039096595` proves typed updates preserve `cstore_typed` |
| Semantics | End-to-end typed assignment theorem | [x] | CI run `32039204250` connects expression evaluation, assignment, and typed store output |
| Semantics | Small-step typed assignment preservation | [x] | CI run `32043962999` proves `small_step_assignment_preserves_store_type` |
| Semantics | C-expression small-step/big-step bridge | [x] | CI run `32045111676` proves `cexpr_step_preserves_big` |
| Semantics | C-expression multi-step/big-step bridge | [x] | CI run `32045271877` proves `cexpr_step_star_preserves_big` |
| Semantics | C-expression reduction closure transitivity | [x] | CI run `32045490645` proves `cexpr_step_star_trans` |
| Semantics | C-statement reduction closure transitivity | [x] | CI run `32045634651` proves `cmstmt_step_star_trans` |
| Semantics | One-step to multi-step reduction embeddings | [x] | CI run `32045916242` proves `cexpr_step_to_star` and `cmstmt_step_to_star` |
| Semantics | Reflexive C reduction closures | [x] | CI run `32050342965` proves `cexpr_step_star_refl` and `cmstmt_step_star_refl` |
| Semantics | Typed assignment configuration step | [x] | CI run `32046091960` proves `cmconfig_typed` and `typed_assignment_config_step` |
| Semantics | Typed assignment multi-step result | [x] | CI run `32046289014` proves `typed_assignment_config_step_star` |
| Semantics | Typed terminal skip configuration | [x] | CI run `32046525816` proves `typed_skip_config` |
| Semantics | Typed assignment big-step result | [x] | CI run `32046679180` proves `typed_assignment_config_big` |
| Semantics | Typed assignment big/small-step agreement | [x] | CI run `32046816812` proves `typed_assignment_semantic_agreement` |
| Semantics | Typed return big/small-step agreement | [x] | CI run `32047001312` proves `typed_return_semantic_agreement` |
| Semantics | Typed skip big/small-step agreement | [x] | CI run `32047143767` proves `typed_skip_semantic_agreement` |
| Semantics | Typed zero-branch `if` agreement | [x] | CI run `32047266224` proves `typed_if_zero_semantic_agreement` |
| Semantics | Typed zero-branch `if` unfolding | [x] | CI run `32050659020` proves `typed_if_zero_unfold` |
| Semantics | Typed nonzero-branch `if` agreement | [x] | CI run `32047519526` proves `typed_if_nonzero_semantic_agreement` |
| Semantics | Typed sequence-skip agreement | [x] | CI run `32047794010` proves `typed_seq_skip_semantic_agreement` |
| Semantics | Typed while-zero unfolding | [x] | CI run `32048209726` proves `typed_while_zero_unfold` |
| Semantics | Typed while-nonzero unfolding | [x] | CI run `32048353146` proves `typed_while_nonzero_unfold` |
| Semantics | Typed while-zero semantic agreement | [x] | CI run `32049691162` proves `typed_while_zero_semantic_agreement` |
| Semantics | Typed while-nonzero semantic agreement | [x] | CI run `32049817907` proves `typed_while_nonzero_semantic_agreement` |
| Semantics | Typed switch-case agreement | [x] | CI run `32048651571` proves `typed_switch_case_semantic_agreement` |
| Semantics | Typed switch-default agreement | [x] | CI run `32049112931` proves `typed_switch_default_semantic_agreement` |
| Semantics | Typed call unfolding | [x] | CI run `32049283090` proves `typed_call_unfold` |
| Semantics | Typed call big-step propagation | [x] | CI run `32049423509` proves `typed_call_big` |
| Semantics | Typed call multi-step unfolding | [x] | CI run `32049556301` proves `typed_call_unfold_star` |
| Semantics | Typed call big/small-step agreement | [x] | CI run `32050498342` proves `typed_call_big_step_agreement` |
| Semantics | Typed call arity invariant | [x] | CI run `32038978602` proves argument/parameter list lengths agree |
| Semantics | Ott source specification | [ ] | Import the syntax and rules into an Ott definition and generate artifacts |
| Semantics | Formal proof of self-applicability | [ ] | Connect staged code, evaluator, and quotation theorem formally |
| Compiler | General C parser beyond recognized fixture shapes | [ ] | Replace regex-shaped parsing with a real parser/AST pipeline |
| Compiler | General expressions and declarations | [ ] | Add typed AST coverage beyond current patterns |
| Compiler | General `switch` AST lowering | [ ] | Current support is specialized to `argc` return forms |
| Compiler | General loop AST lowering | [ ] | Current `break`/`continue` support is counted-loop specialized |
| Compiler | Macro/preprocessor support | [ ] | Explicitly out of current subset scope |
| Compiler | General C ABI and variadic calls | [ ] | Add ABI lowering and call-frame validation |
| Compiler | Portable non-x86-64 backends | [ ] | Current target is Linux x86-64 assembly |
| CLI compatibility | Complete `ls` compatibility | [ ] | Expand options, permissions, symlinks, and formatting |
| CLI compatibility | Complete `find` compatibility | [ ] | Expand predicates, actions, traversal, and expressions |
| CLI compatibility | Complete `echo` compatibility | [ ] | Expand quoting/options and shell behavior |
| Publication | Git repository push | [x] | `origin/main` is updated continuously |
| Publication | Cloudflared artifact endpoint | [x] | Endpoint checked with HTTP 200 after recent pushes |
| Publication | Reproducible release artifact bundle | [x] | `make_release_bundle.sh` creates a source archive, manifest, and archive checksum |

## Immediate next work

- [ ] Replace the two remaining admitted Coq theorems with checked proofs.
- [ ] Install Ott in CI and generate/check artifacts from `semantics.ott`.
- [ ] If broader C support is required, replace regex-shaped parsing with general AST nodes.
- [x] Produce a reproducible artifact bundle with checksums through Cloudflared.

## Minimal functional finish line

The current minimal functional implementation is the assembler-only x86-64
pipeline plus the dependent NbE core: it builds, rejects malformed fixtures,
assembles and links without GCC, runs the full fixture regression, and checks
the available Coq sources. The remaining unchecked rows are formal-completeness
or scope-expansion work. They should not be conflated with a broken runtime:
the two admitted Coq theorems and Ott generation are required for a fully
formalized release, while a general C parser and full CLI compatibility are
separate expansion projects.

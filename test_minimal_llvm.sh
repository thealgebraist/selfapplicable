#!/bin/sh
set -eu
compiler=${1:-./minimal_llvm_compiler-ci}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
"$compiler" examples/selfapp_llvm.min > "$tmpdir/out.ll"
llvm-as "$tmpdir/out.ll" -o "$tmpdir/out.bc"
grep -q 'ret i64 42' "$tmpdir/out.ll"
echo "minimal LLVM self-application: PASS (42)"
"$compiler" examples/nontrivial_llvm.min > "$tmpdir/nontrivial.ll"
llvm-as "$tmpdir/nontrivial.ll" -o "$tmpdir/nontrivial.bc"
grep -q 'ret i64 82' "$tmpdir/nontrivial.ll"
echo "minimal LLVM nontrivial program: PASS (82)"

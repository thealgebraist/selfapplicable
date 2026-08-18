#!/bin/sh
set -eu
compiler=${1:-./minimal_llvm_compiler-ci}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
"$compiler" examples/selfapp_llvm.min > "$tmpdir/out.ll"
llvm-as "$tmpdir/out.ll" -o "$tmpdir/out.bc"
grep -q 'ret i64 42' "$tmpdir/out.ll"
echo "minimal LLVM self-application: PASS (42)"

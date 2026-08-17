#!/bin/sh
set -eu
compiler=${1:-./minimal_arm64_compiler-ci}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

printf '%s\n' '(add (nat 2) (if true (nat 3) (nat 4)))' > "$tmpdir/input.min"
"$compiler" "$tmpdir/input.min" > "$tmpdir/output.s"
grep -q '^    mov x0, #5$' "$tmpdir/output.s"
grep -q '^    mov x8, #93$' "$tmpdir/output.s"
direct=$($compiler --direct "$tmpdir/input.min" | sha256sum | cut -d ' ' -f 1)
staged=$($compiler "$tmpdir/input.min" | sha256sum | cut -d ' ' -f 1)
test "$direct" = "$staged"
if command -v aarch64-linux-gnu-as >/dev/null 2>&1; then
  aarch64-linux-gnu-as "$tmpdir/output.s" -o "$tmpdir/output.o"
fi
echo 'minimal ARM64 compiler: ok'

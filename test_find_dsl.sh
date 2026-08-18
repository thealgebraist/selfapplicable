#!/bin/sh
set -eu
compiler=${1:-./find_dsl_compiler-ci}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
mkdir -p "$tmpdir/root/skip" "$tmpdir/root/sub"
touch "$tmpdir/root/a.c" "$tmpdir/root/skip/hidden.c" "$tmpdir/root/sub/c.c"
sed "s#ROOT#$tmpdir/root#g" examples/find_skip.find > "$tmpdir/query.find"
actual=$($compiler "$tmpdir/query.find" 2> "$tmpdir/stage.log" | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/skip" "$tmpdir/root/sub/c.c" | sort)
test "$actual" = "$expected"
grep -q 'quoted, normalized, and executing checked ADT' "$tmpdir/stage.log"
echo "Find DSL compiler/backend: PASS"

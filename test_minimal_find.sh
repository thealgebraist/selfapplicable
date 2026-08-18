#!/bin/sh
set -eu
compiler=${1:-./minimal_find_cli-ci}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
mkdir -p "$tmpdir/root/sub"
touch "$tmpdir/root/a.c" "$tmpdir/root/b.txt" "$tmpdir/root/sub/c.c"
actual=$($compiler "$tmpdir/root" -name '*.c' -type f | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/sub/c.c" | sort)
test "$actual" = "$expected"
actual=$($compiler "$tmpdir/root" -type f -maxdepth 1 | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/b.txt" | sort)
test "$actual" = "$expected"
echo "ADT find CLI: PASS"

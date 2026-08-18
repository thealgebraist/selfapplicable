#!/bin/sh
set -eu
compiler=${1:-./minimal_find_cli-ci}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
mkdir -p "$tmpdir/root/sub"
mkdir -p "$tmpdir/root/empty"
touch "$tmpdir/root/a.c" "$tmpdir/root/b.txt" "$tmpdir/root/sub/c.c"
actual=$($compiler "$tmpdir/root" -name '*.c' -type f | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/sub/c.c" | sort)
test "$actual" = "$expected"
actual=$($compiler "$tmpdir/root" -type f -maxdepth 1 | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/b.txt" | sort)
test "$actual" = "$expected"
printf x > "$tmpdir/root/one"
actual=$($compiler "$tmpdir/root" -size 1c -type f | sort)
test "$actual" = "$tmpdir/root/one"
actual=$($compiler "$tmpdir/root" -name '*.c' -o -name '*.txt' | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/b.txt" "$tmpdir/root/sub/c.c" | sort)
test "$actual" = "$expected"
actual=$($compiler "$tmpdir/root" -empty -type d | sort)
test "$actual" = "$tmpdir/root/empty"
actual=$($compiler "$tmpdir/root" -iname '*.C' -type f | sort)
expected=$(printf '%s\n' "$tmpdir/root/a.c" "$tmpdir/root/sub/c.c" | sort)
test "$actual" = "$expected"
actual=$($compiler "$tmpdir/root" -path "$tmpdir/root/sub/*.c" -type f)
test "$actual" = "$tmpdir/root/sub/c.c"
echo "ADT find CLI: PASS"

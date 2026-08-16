#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT INT TERM

g++ -std=c++23 -Wall -Wextra -pedantic "$root/c_subset_compiler.cpp" -o "$tmp/c_subset_compiler"

expect_status() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  "$tmp/$stem"
  actual=$?
  set -e
  test "$actual" -eq "$expected" || {
    echo "FAIL: $source: expected exit $expected, got $actual" >&2
    exit 1
  }
}

expect_reject() {
  source=$1
  if "$tmp/c_subset_compiler" "$root/$source" > "$tmp/reject.s" 2>/dev/null; then
    echo "FAIL: $source was accepted" >&2
    exit 1
  fi
}

expect_status fixtures/echo.c 0
expect_status fixtures/function_call.c 7
expect_status fixtures/function_pointer.c 6
expect_status fixtures/function_pointer_global.c 6
expect_status fixtures/function_add.c 7
expect_status fixtures/function_constant.c 9
expect_status fixtures/struct_field_return.c 7
expect_status fixtures/struct_arrow_return.c 8
expect_status fixtures/struct_next_return.c 12
expect_status fixtures/pointer_array_assign.c 16
expect_status fixtures/pointer_add_deref.c 17
expect_status fixtures/array_sizeof_return.c 12
expect_status fixtures/struct_sizeof_return.c 12
expect_status fixtures/node_offsetof_return.c 4
expect_status fixtures/null_guard.c 1
expect_status fixtures/recursive_base_case.c 0
expect_status fixtures/global_return.c 4

expect_reject fixtures/unknown_call.c
expect_reject fixtures/function_add_bad_arity.c
expect_reject fixtures/function_pointer_bad_arity.c
expect_reject fixtures/struct_value_cycle.c
expect_reject fixtures/duplicate_global.c
expect_reject fixtures/duplicate_pointer_global.c
expect_reject fixtures/array_index_bad.c

echo "assembler regression: PASS"

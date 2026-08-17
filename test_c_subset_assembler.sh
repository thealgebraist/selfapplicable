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
expect_status fixtures/function_pointer_address.c 6
expect_status fixtures/function_pointer_explicit_deref.c 6
expect_status fixtures/function_pointer_global.c 6
expect_status fixtures/function_pointer_global_address.c 6
expect_status fixtures/function_pointer_global_explicit_deref.c 6
expect_status fixtures/function_pointer_global_binary.c 7
expect_status fixtures/function_pointer_global_binary_direct.c 7
expect_status fixtures/function_pointer_global_nullary.c 9
expect_status fixtures/function_pointer_global_nullary_direct.c 9
expect_status fixtures/function_pointer_nullary.c 9
expect_status fixtures/function_pointer_nullary_explicit_deref.c 9
expect_status fixtures/function_pointer_binary.c 7
expect_status fixtures/function_pointer_binary_explicit_deref.c 7
expect_status fixtures/function_pointer_alias.c 6
expect_status fixtures/function_pointer_binary_alias.c 7
expect_status fixtures/function_pointer_binary_alias_explicit_deref.c 7
expect_status fixtures/function_pointer_nullary_alias.c 9
expect_status fixtures/function_pointer_nested.c 9
expect_status fixtures/function_pointer_nested_unary.c 6
expect_status fixtures/function_pointer_nested_binary.c 7
expect_status fixtures/function_pointer_parameter.c 6
expect_status fixtures/function_pointer_binary_parameter.c 7
expect_status fixtures/function_pointer_typedef.c 6
expect_status fixtures/function_pointer_binary_typedef.c 7
expect_status fixtures/function_pointer_nullary_typedef.c 9
expect_status fixtures/function_pointer_typedef_parameter.c 6
expect_status fixtures/function_pointer_binary_typedef_parameter.c 7
expect_status fixtures/function_pointer_void.c 0
expect_status fixtures/function_pointer_void_binary.c 0
expect_status fixtures/function_pointer_void_typedef.c 0
expect_status fixtures/function_pointer_void_binary_typedef.c 0
expect_status fixtures/function_pointer_void_parameter.c 0
expect_status fixtures/function_pointer_void_binary_parameter.c 0
expect_status fixtures/function_pointer_equality.c 1
expect_status fixtures/function_pointer_struct_field.c 6
expect_status fixtures/function_pointer_struct_binary_field.c 7
expect_status fixtures/function_pointer_struct_void_field.c 0
expect_status fixtures/function_pointer_struct_nullary_field.c 9
expect_status fixtures/function_pointer_struct_typedef_field.c 6
expect_status fixtures/function_pointer_struct_arrow_field.c 6
expect_status fixtures/function_pointer_struct_arrow_binary_field.c 7
expect_status fixtures/function_pointer_struct_arrow_void_field.c 0
expect_status fixtures/function_pointer_struct_arrow_nullary_field.c 9
expect_status fixtures/function_pointer_struct_aggregate.c 6
expect_status fixtures/function_pointer_struct_binary_aggregate.c 7
expect_status fixtures/function_pointer_struct_nullary_aggregate.c 9
expect_status fixtures/function_pointer_struct_void_aggregate.c 0
expect_status fixtures/function_pointer_struct_typedef_aggregate.c 6
expect_status fixtures/function_pointer_struct_binary_typedef_aggregate.c 7
expect_status fixtures/function_pointer_struct_void_typedef_aggregate.c 0
expect_status fixtures/function_pointer_struct_void_binary_typedef_aggregate.c 0
expect_status fixtures/function_pointer_struct_typedef_arrow.c 6
expect_status fixtures/function_pointer_struct_binary_typedef_arrow.c 7
expect_status fixtures/function_pointer_struct_void_typedef_arrow.c 0
expect_status fixtures/function_pointer_struct_multiple_fields.c 6
expect_status fixtures/function_pointer_struct_mixed_fields.c 6
expect_status fixtures/function_pointer_struct_mixed_binary_fields.c 7
expect_status fixtures/function_pointer_struct_mixed_aggregate.c 6
expect_status fixtures/function_pointer_struct_mixed_binary_aggregate.c 7
expect_status fixtures/function_pointer_struct_nested_field.c 6
expect_status fixtures/switch_return.c 7
expect_status fixtures/switch_default_return.c 3
expect_status fixtures/switch_two_cases.c 7
expect_status fixtures/enum_switch.c 7
expect_status fixtures/enum_switch_implicit.c 8
expect_status fixtures/bitwise_return.c 2
expect_status fixtures/shift_return.c 4
expect_status fixtures/modulo_return.c 1
expect_reject fixtures/modulo_by_zero.c
expect_status fixtures/division_return.c 3
expect_reject fixtures/division_by_zero.c
expect_status fixtures/comparison_return.c 1
expect_status fixtures/logical_and_return.c 0
expect_status fixtures/logical_or_return.c 1
expect_status fixtures/ternary_true_return.c 7
expect_status fixtures/ternary_false_return.c 3
expect_status fixtures/logical_not_return.c 1
expect_status fixtures/negative_return.c 253
expect_status fixtures/addition_return.c 7
expect_status fixtures/subtraction_return.c 5
expect_status fixtures/multiplication_return.c 12
expect_status fixtures/char_sizeof_return.c 1
expect_status fixtures/char_pointer_sizeof_return.c 8
expect_status fixtures/character_literal_return.c 65
expect_status fixtures/character_escape_return.c 10
expect_status fixtures/character_tab_return.c 9
expect_status fixtures/character_quote_return.c 39
expect_status fixtures/character_backslash_return.c 92
expect_status fixtures/character_hex_return.c 65
expect_status fixtures/character_octal_return.c 65
expect_reject fixtures/character_bad_hex.c
expect_reject fixtures/character_bad_octal.c
expect_reject fixtures/character_multi_byte.c
expect_output() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  actual=$("$tmp/$stem")
  test "$actual" = "$expected" || { echo "FAIL: $source: output mismatch" >&2; exit 1; }
}
expect_output fixtures/write_escape.c "A	B"
expect_output fixtures/loop_write_escape.c "x	x	"
expect_output fixtures/loop_write_hex.c "AA"
expect_output fixtures/loop_write_quote.c 'A"BA"B'
expect_output fixtures/loop_write_apostrophe.c "A'BA'B"
expect_output fixtures/loop_write_question.c "A?BA?B"
expect_output fixtures/loop_write_adjacent.c "ABAB"
expect_output fixtures/loop_write_adjacent_hex.c "ABAB"
expect_output fixtures/loop_write_adjacent_mixed.c "?A?A"
expect_output fixtures/loop_write_adjacent_octal.c "ABAB"
expect_output fixtures/loop_write_adjacent_three.c "ABCABC"
expect_output fixtures/loop_write_adjacent_four.c "ABCDABCD"
expect_output fixtures/loop_write_adjacent_five.c "ABCDEABCDE"
expect_output fixtures/write_backslash.c "A\\B"
expect_output fixtures/write_quote.c 'A"B'
expect_output fixtures/write_two.c "AB"
expect_output fixtures/write_three.c "ABC"
expect_bytes() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  actual=$("$tmp/$stem" | od -An -tx1 | tr -d ' \n')
  test "$actual" = "$expected" || { echo "FAIL: $source: byte mismatch" >&2; exit 1; }
}
expect_bytes fixtures/loop_write_adjacent_control.c "07080708"
expect_bytes fixtures/loop_write_adjacent_carriage_return.c "0d410d41"
expect_bytes fixtures/write_adjacent_nul.c "410042"
expect_bytes fixtures/write_adjacent_high_byte.c "ff41"
expect_bytes fixtures/write_adjacent_octal.c "4142"
expect_bytes fixtures/write_adjacent_control.c "0708"
expect_bytes fixtures/write_adjacent_whitespace.c "0a09"
expect_bytes fixtures/write_adjacent_form_vertical.c "0c0b"
expect_bytes fixtures/write_adjacent_binary_mixed.c "00ff"
expect_output fixtures/write_adjacent_mixed.c "?A"
expect_output fixtures/write_adjacent_empty.c "A"
expect_output fixtures/write_adjacent_empty_trailing.c "A"
expect_output fixtures/write_adjacent_empty_both.c "A"
expect_bytes fixtures/write_adjacent_mixed_whitespace.c "0a41"
expect_bytes fixtures/write_adjacent_mixed_tab_hex.c "0941"
expect_bytes fixtures/write_adjacent_mixed_cr_hex.c "0d41"
expect_bytes fixtures/write_adjacent_mixed_form_hex.c "0c41"
expect_bytes fixtures/write_adjacent_mixed_vertical_hex.c "0b41"
expect_bytes fixtures/write_adjacent_mixed_apostrophe_hex.c "2741"
expect_bytes fixtures/write_adjacent_mixed_backslash_hex.c "5c41"
expect_bytes fixtures/write_adjacent_mixed_backslash_octal.c "5c41"
expect_bytes fixtures/loop_write_adjacent_mixed_octal.c "41094109"
expect_bytes fixtures/loop_write_adjacent_binary_mixed.c "410041410041"
expect_bytes fixtures/loop_write_adjacent_form_vertical.c "0c0b0c0b"
expect_bytes fixtures/loop_write_high_byte.c "ffff"
expect_bytes fixtures/loop_write_octal.c "4141"
expect_bytes fixtures/loop_write_hex_single.c "045a045a"
expect_bytes fixtures/write_carriage_return.c "410d42"
expect_bytes fixtures/loop_write_carriage_return.c "410d42410d42"
expect_bytes fixtures/loop_write_control_escapes.c "07080c0b07080c0b"
expect_bytes fixtures/loop_write_nul.c "410042410042"
expect_bytes fixtures/loop_write_adjacent_nul.c "410042410042"
expect_bytes fixtures/loop_write_adjacent_high_byte.c "ff41ff41"
expect_bytes fixtures/loop_write_adjacent_newline.c "0a410a41"
expect_bytes fixtures/loop_write_adjacent_tab.c "09410941"
expect_bytes fixtures/write_control_escapes.c "07080c0b"
expect_bytes fixtures/write_nul.c "410042"
expect_bytes fixtures/write_octal_string.c "414142"
expect_bytes fixtures/write_hex_string.c "414243"
expect_bytes fixtures/write_apostrophe.c "412742"
expect_bytes fixtures/write_high_byte.c "ff"
expect_bytes fixtures/write_question_escape.c "413f42"
expect_bytes fixtures/write_hex_single.c "41045a"
expect_bytes fixtures/write_adjacent.c "4142"
expect_bytes fixtures/write_adjacent_three.c "414243"
expect_bytes fixtures/write_adjacent_four.c "41424344"
expect_bytes fixtures/write_adjacent_five.c "4142434445"
expect_status fixtures/enum_sizeof_return.c 4
expect_reject fixtures/enum_sizeof_undeclared.c
expect_reject fixtures/enum_switch_duplicate.c
expect_reject fixtures/enum_switch_duplicate_name.c
expect_reject fixtures/function_pointer_alias_bad_type.c
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
expect_reject fixtures/function_pointer_global_bad_init.c
expect_reject fixtures/function_pointer_explicit_deref_bad_arity.c
expect_reject fixtures/function_pointer_global_binary_bad_arity.c
expect_reject fixtures/function_pointer_binary_bad_arity.c
expect_reject fixtures/function_pointer_global_binary_direct_bad_arity.c
expect_reject fixtures/function_pointer_global_nullary_bad_arity.c
expect_reject fixtures/struct_value_cycle.c
expect_reject fixtures/duplicate_global.c
expect_reject fixtures/duplicate_pointer_global.c
expect_reject fixtures/array_index_bad.c
expect_reject fixtures/write_bad_hex_string.c
expect_reject fixtures/write_bad_octal_string.c
expect_reject fixtures/loop_write_bad_hex.c
expect_reject fixtures/loop_write_bad_octal.c
expect_reject fixtures/write_adjacent_bad_hex.c
expect_reject fixtures/write_adjacent_bad_hex_digits.c
expect_reject fixtures/write_adjacent_bad_octal_digits.c
expect_reject fixtures/write_adjacent_bad_octal.c
expect_reject fixtures/write_adjacent_bad_escape.c
expect_reject fixtures/loop_write_adjacent_bad_escape.c

echo "assembler regression: PASS"

#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if command -v coqc >/dev/null 2>&1; then
  coqc -q -Q "$root" SelfApp "$root/semantics.v"
  rm -f "$root/semantics.vo" "$root/semantics.glob" "$root/semantics.vok" "$root/semantics.vos"
  echo "Coq semantics: checked"
else
  echo "Coq semantics: skipped (coqc is not installed)" >&2
fi

if command -v ott >/dev/null 2>&1; then
  ott -i "$root/semantics.ott" -o "$root/.semantics-generated.coq" -coq
  rm -f "$root/.semantics-generated.coq"
  echo "Ott specification: checked"
else
  echo "Ott specification: skipped (ott is not installed)" >&2
fi

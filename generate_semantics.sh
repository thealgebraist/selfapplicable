#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ott_bin=${OTT_BIN:-ott}

if ! command -v "$ott_bin" >/dev/null 2>&1; then
  echo "generate_semantics: Ott executable not found" >&2
  echo "install Ott from https://github.com/ott-lang/ott or set OTT_BIN" >&2
  exit 2
fi

out=${1:-"$root/generated-semantics.tex"}
exec "$ott_bin" -i "$root/semantics.ott" -o "$out"

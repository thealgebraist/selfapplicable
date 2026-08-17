#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
version=${1:-$(git -C "$root" rev-parse --short HEAD)}
out=${2:-"$root/dist"}
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT INT TERM

bundle="selfapplicable-$version"
mkdir -p "$stage/$bundle"
git -C "$root" archive --format=tar --prefix="$bundle/" HEAD | tar -xf - -C "$stage"

manifest="$stage/$bundle/MANIFEST.sha256"
(
  cd "$stage/$bundle"
  find . -type f ! -name MANIFEST.sha256 -print | sort | while IFS= read -r path; do
    sha256sum "$path"
  done
) > "$manifest"

mkdir -p "$out"
tar -C "$stage" -czf "$out/$bundle.tar.gz" "$bundle"
sha256sum "$out/$bundle.tar.gz" > "$out/$bundle.tar.gz.sha256"
printf '%s\n' "$out/$bundle.tar.gz" "$out/$bundle.tar.gz.sha256"

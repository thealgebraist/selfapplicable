#!/bin/sh
set -eu

archive=${1:?usage: verify_release_bundle.sh ARCHIVE.tar.gz}
checksum="$archive.sha256"
test -f "$checksum" || {
  echo "missing checksum: $checksum" >&2
  exit 2
}
sha256sum -c "$checksum"

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT INT TERM
tar -xzf "$archive" -C "$stage"

manifest=$(find "$stage" -name MANIFEST.sha256 -type f -print -quit)
test -n "$manifest" || {
  echo "archive has no MANIFEST.sha256" >&2
  exit 1
}
bundle_dir=$(dirname "$manifest")
(cd "$bundle_dir" && sha256sum -c MANIFEST.sha256)
echo "release bundle verified: $archive"

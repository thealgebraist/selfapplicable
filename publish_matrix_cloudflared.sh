#!/bin/sh
set -eu

# Serve the repository directly so WORK_MATRIX.md is refreshed on every HTTP
# request. Cloudflared supplies the public endpoint; no copied snapshot is
# used, so edits become visible without rebuilding a bundle.
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
port=${MATRIX_PORT:-8000}
cloudflared_bin=${CLOUDFLARED_BIN:-cloudflared}

if ! curl -fsS "http://127.0.0.1:${port}/WORK_MATRIX.html" >/dev/null 2>&1; then
  python3 "$repo_dir/matrix_server.py" --port "$port" \
    >"${TMPDIR:-/tmp}/selfapp-matrix-http-${port}.log" 2>&1 &
  server_pid=$!
  trap 'kill "$server_pid" 2>/dev/null || true' EXIT INT TERM
  i=0
  while ! curl -fsS "http://127.0.0.1:${port}/WORK_MATRIX.html" >/dev/null 2>&1; do
    i=$((i + 1))
    [ "$i" -lt 50 ] || exit 1
    sleep 0.1
  done
fi

exec "$cloudflared_bin" tunnel --url "http://127.0.0.1:${port}"

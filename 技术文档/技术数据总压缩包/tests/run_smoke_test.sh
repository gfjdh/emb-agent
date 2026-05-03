#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/ncnnv5lite"
SRC="$ROOT/source"
IMG="$SRC/images/horse.jpg"
OUT_DIR="$ROOT/tests/output"

mkdir -p "$OUT_DIR"

if [[ ! -x "$BIN" ]]; then
  echo "FAIL: executable not found: $BIN" >&2
  exit 1
fi

if [[ ! -f "$IMG" ]]; then
  echo "FAIL: sample image not found: $IMG" >&2
  exit 1
fi

pushd "$SRC" >/dev/null
"$BIN" "$IMG"
popd >/dev/null

if [[ -f "$SRC/result.jpg" ]]; then
  mv "$SRC/result.jpg" "$OUT_DIR/result.jpg"
  echo "PASS: result.jpg generated at $OUT_DIR/result.jpg"
else
  echo "FAIL: result.jpg not generated" >&2
  exit 2
fi

#!/bin/bash
# Rebuild the macOS product PKG after CPack staging.
#
# CPack productbuild runs pkgbuild --root on the staging directory after
# creating Contents/Packages inside it, which incorrectly bundles
# Contents/ into the payload (installer then writes to /Contents/... on the
# system volume and fails). This script rebuilds the component and product
# packages from a clean payload containing only usr/ and Library/.

set -euo pipefail

PROJECT_NAME="${1:-simple-rsyncd}"
VERSION="${2:-0.3.0}"
BUILD_DIR="${3:-build}"

shopt -s nullglob
stages=("$BUILD_DIR/_CPack_Packages/Darwin/productbuild/${PROJECT_NAME}-"*)
shopt -u nullglob

if [ ${#stages[@]} -eq 0 ]; then
    echo "error: no CPack productbuild staging directory under $BUILD_DIR" >&2
    exit 1
fi

STAGE="${stages[0]}"
PKG_BASENAME="$(basename "$STAGE")"
OUT_PKG="$BUILD_DIR/${PKG_BASENAME}.pkg"
PAYLOAD="$(mktemp -d)"
IDENTIFIER="com.simpledaemons.${PROJECT_NAME}"

cleanup() {
    rm -rf "$PAYLOAD"
}
trap cleanup EXIT

if [ ! -d "$STAGE/usr" ] && [ ! -d "$STAGE/Library" ]; then
    echo "error: staging directory missing usr/ or Library/: $STAGE" >&2
    exit 1
fi

mkdir -p "$PAYLOAD"
[ -d "$STAGE/usr" ] && cp -R "$STAGE/usr" "$PAYLOAD/"
[ -d "$STAGE/Library" ] && cp -R "$STAGE/Library" "$PAYLOAD/"

mkdir -p "$STAGE/Contents/Packages"
pkgbuild --root "$PAYLOAD" \
    --install-location / \
    --identifier "$IDENTIFIER" \
    --version "$VERSION" \
    "$STAGE/Contents/Packages/${PROJECT_NAME}"

productbuild --distribution "$STAGE/Contents/distribution.dist" \
    --package-path "$STAGE/Contents/Packages" \
    --resources "$STAGE/Contents" \
    --version "$VERSION" \
    --identifier "$IDENTIFIER" \
    "$OUT_PKG"

echo "Rebuilt macOS PKG: $OUT_PKG"

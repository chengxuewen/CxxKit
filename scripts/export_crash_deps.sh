#!/usr/bin/env bash
# CxxKit crash-deps export script (development-time tool, run on a vcpkg machine).
#
# Produces:
#   <OUT_DIR>/<OUT_NAME>.7z          — vcpkg export of breakpad/backward-cpp/elfutils/libunwind
#   <OUT_DIR>/<OUT_NAME>.7z.version  — sidecar with the four packages' resolved versions
#                                      (consumed by FindWrapCrashDeps.cmake as a version gate)
#
# Usage:
#   bash scripts/export_crash_deps.sh [TRIPLET] [OUT_NAME] [OUT_DIR]
#     TRIPLET  overlay triplet (default x64-linux-cxxkit; internal install/export only, -fPIC)
#     OUT_NAME standard-triplet output name (default crash-deps-x64-linux;
#              matches the FindWrap triplet map — the overlay triplet name never escapes)
#     OUT_DIR  output dir (default <repo>/3rdparty)
#   Env: VCPKG_ROOT (default $HOME/vcpkg)
#
# The produced .7z + .version are build caches, not sources: they are gitignored.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRIPLET="${1:-x64-linux-cxxkit}"
OUT_NAME="${2:-crash-deps-x64-linux}"
OUT_DIR="${3:-$SCRIPT_DIR/../3rdparty}"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
VCPKG="$VCPKG_ROOT/vcpkg"
MANIFEST_DIR="$SCRIPT_DIR/crash-deps"
INSTALLED="$VCPKG_ROOT/installed"

[ -x "$VCPKG" ] || { echo "FATAL: vcpkg not found at $VCPKG (install it or set VCPKG_ROOT)" >&2; exit 1; }
[ -f "$MANIFEST_DIR/vcpkg.json" ] || { echo "FATAL: manifest missing at $MANIFEST_DIR/vcpkg.json" >&2; exit 1; }
mkdir -p "$OUT_DIR"

# 1) install with the overlay triplet (-fPIC)
"$VCPKG" install --triplet "$TRIPLET" --overlay-triplets="$MANIFEST_DIR" --x-manifest-root="$MANIFEST_DIR"

# 2) license assertion: every package must ship its copyright file (R-M5)
for pkg in breakpad backward-cpp elfutils libunwind; do
    if [ ! -f "$INSTALLED/$TRIPLET/share/$pkg/copyright" ]; then
        echo "FATAL: $INSTALLED/$TRIPLET/share/$pkg/copyright missing (license not shipped)" >&2
        exit 1
    fi
done

# 3) export (--7zip: vcpkg manages its own 7zip tool, no system 7z required)
rm -f "$OUT_DIR/$OUT_NAME.7z" "$OUT_DIR/$OUT_NAME.7z.version"
"$VCPKG" export --7zip --triplet "$TRIPLET" --overlay-triplets="$MANIFEST_DIR" \
    --x-manifest-root="$MANIFEST_DIR" --output-dir="$OUT_DIR" \
    breakpad backward-cpp elfutils libunwind

# 4) rename the produced .7z to the standard-triplet name (R4: FindWrap maps by it)
exported_count="$(ls "$OUT_DIR"/*.7z 2>/dev/null | wc -l)"
if [ "$exported_count" -ne 1 ]; then
    echo "FATAL: expected exactly one .7z from vcpkg export, found $exported_count in $OUT_DIR" >&2
    exit 1
fi
exported="$(ls -t "$OUT_DIR"/*.7z 2>/dev/null | head -1 || true)"
if [ -z "$exported" ] || [ ! -f "$exported" ]; then
    echo "FATAL: vcpkg export produced no .7z in $OUT_DIR" >&2
    exit 1
fi
mv -f "$exported" "$OUT_DIR/$OUT_NAME.7z"
exported="$(ls -t "$OUT_DIR"/*.7z 2>/dev/null | head -1 || true)"
if [ -z "$exported" ] || [ ! -f "$exported" ]; then
    echo "FATAL: vcpkg export produced no .7z in $OUT_DIR" >&2
    exit 1
fi
mv -f "$exported" "$OUT_DIR/$OUT_NAME.7z"

# 5) sidecar: resolved versions from installed/vcpkg/info/<pkg>_*.list
#    (filename format: <pkg>_<version>[_<port-version>]_<triplet>.list)
for pkg in breakpad backward-cpp elfutils libunwind; do
    f="$(ls "$INSTALLED/vcpkg/info/${pkg}"_*.list 2>/dev/null | head -1 || true)"
    if [ -z "$f" ] || [ ! -f "$f" ]; then
        echo "FATAL: no installed/vcpkg/info/${pkg}_*.list — package not installed?" >&2
        exit 1
    fi
    mid="$(basename "$f")"
    mid="${mid#"$pkg"_}"
    mid="${mid%_$TRIPLET.list}"
    if [[ "$mid" == *_* ]]; then
        echo "${pkg}=${mid%_*}#${mid##*_}"
    else
        echo "${pkg}=${mid}"
    fi
done > "$OUT_DIR/$OUT_NAME.7z.version"

echo "OK: $OUT_DIR/$OUT_NAME.7z + $OUT_DIR/$OUT_NAME.7z.version"

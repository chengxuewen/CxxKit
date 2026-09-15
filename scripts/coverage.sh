#!/usr/bin/env bash
# CxxKit coverage report generator (gcov, no lcov/gcovr needed).
# Usage:  bash scripts/coverage.sh <build_dir>
# Requires CXXKIT_BUILD_COVERAGE=ON build + suite run (produces .gcda).
# Runs `gcov -b -c <file>.gcda` per object — gcov prints "Lines executed:X% of N"
# to stdout for the primary compiland — aggregates those into
# <build>/coverage/summary.txt and prints a line-weighted aggregate + per-file table.
set -euo pipefail

BUILD_DIR="${1:?usage: scripts/coverage.sh <build_dir>}"
COV_DIR="$BUILD_DIR/coverage"
mkdir -p "$COV_DIR"
COV_SUM="$COV_DIR/summary.txt"
: > "$COV_SUM"

# 1) run gcov per .cpp .gcda; capture only the primary unit's "Lines executed" line
#    (prefixed by "File '<...>/<unit>.cpp'" so we don't take .h/.tcc summaries).
find "$BUILD_DIR" -name '*.cpp.gcda' -print0 2>/dev/null | while IFS= read -r -d '' gcda; do
    unit="$(basename "$gcda" .gcda)"
    # exclude the build tree's own vendored 3rdparty and the tests (we want library .cpp only);
    # do NOT match the repo's source path /3rdparty/ (CxxKit lives under src/3rdparty/).
    case "$gcda" in
        "$BUILD_DIR"/3rdparty/*|"$BUILD_DIR"/tests/*) continue ;;
    esac
    # Upstream imgui 5 units are compiled INTO cxxkit_imgui (extract-only wrap, decision 8 of
    # the imgui P0 plan): their .gcda lands under cxxkit/imgui/ so the path filter above does
    # not hit. Vendored upstream code does not belong to cxxkit's own coverage gate (libyuv
    # precedent: it is built in its wrap dir and excluded by path).
    # Coverage-wave addition (B wave): the imgui/SDL platform files are display-host code —
    # unreachable in a headless CI run by construction (D40/D42: sdl backend + application own
    # the window lifecycle; cxxkit-never-opens-windows). Include the imgui upstream demo-family
    # list AND the platform/backend files in the exclusion set so the gate measures cxxkit's
    # own testable library code.
    case "$unit" in
        imgui.cpp|imgui_draw.cpp|imgui_tables.cpp|imgui_widgets.cpp|imgui_demo.cpp|\
        imgui_impl_opengl3.cpp|imgui_impl_sdl3.cpp|imgui_impl_glfw.cpp|imgui_impl_vulkan.cpp|\
        imgui_impl_dx11.cpp|imgui_impl_dx12.cpp|imgui_impl_win32.cpp|imgui_impl_allegro5.cpp|\
        sdl3_backend.cpp|sdl_application.cpp) continue ;;
    esac
    out="$(cd "$(dirname "$gcda")" && gcov -b -c "$(basename "$gcda")" 2>/dev/null || true)"
    exec_line="$(printf '%s\n' "$out" | awk -v u="${unit}" '
        /^File .*/            { if (index($0,"/" u) > 0) cur=1; else cur=0; next }
                              { if (cur && $0 ~ /Lines executed:/) { print; exit } }')"
    [ -n "$exec_line" ] || continue
    pct="$(echo "$exec_line" | sed -E 's/.*executed:([0-9.]+)% of .*/\1/')"
    total="$(echo "$exec_line" | sed -E 's/.*executed:[0-9.]+% of ([0-9]+).*/\1/')"
    echo "$pct $total ${unit}" >> "$COV_SUM"
done

echo "===== CxxKit gcov coverage summary (per library .cpp) ====="
awk '{pct=$1; tot=$2; exec+=pct/100*tot; total+=tot;
      if(n==0||pct<min)min=pct; if(pct>max)max=pct; n++}
END{
    printf "Line-weighted aggregate:\t%.1f%%  (%d exec / %d total, %d files)\n", (total?exec/total*100:0), exec, total, n;
    printf "Per-file range:\t\t\t%.1f%% .. %.1f%%\n", min, max;
}' "$COV_SUM"

echo "===== sorted per-file (top 60) ====="
sort -t' ' -k1 -rn "$COV_SUM" | head -n 60

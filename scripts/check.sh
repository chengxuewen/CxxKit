#!/usr/bin/env bash
# cxxkit 本地质量门禁：format + namespace + build + test
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== 1/4 clang-format 检查 ==="
FILES=$(find cxxkit src tests -name "*.hpp" -o -name "*.cpp" | sort)
if command -v clang-format >/dev/null; then
    clang-format --dry-run --Werror $FILES || { echo "格式不合格，请运行: clang-format -i <文件>"; exit 1; }
    echo "clang-format OK"
else
    echo "跳过（clang-format 未安装）"
fi

echo "=== 2/4 命名空间检查（octk 残留）==="
if grep -rn "octk\|OCTK_" cxxkit/ src/ tests/ --include="*.hpp" --include="*.cpp" | grep -vE "CXXKIT|octk mechanism|from OpenCTK|Ported from|Slimmed from"; then
    echo "发现 octk 残留！"; exit 1
fi
echo "namespace OK"

echo "=== 3/4 构建 ==="
cmake -S . -B build -G Ninja -DCXXKIT_BUILD_TESTS=ON -DCXXKIT_ENABLE_LIB_NETWORK=ON
cmake --build build --parallel 4

echo "=== 4/4 测试 ==="
ctest --test-dir build --output-on-failure
echo "=== ALL CHECKS PASSED ==="

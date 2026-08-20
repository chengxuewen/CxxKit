#!/usr/bin/env bash
# CxxKit 本地质量门禁：format + namespace + build + test
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== 1/5 clang-format 检查 ==="
FILES=$(find cxxkit tests -name "*.hpp" -o -name "*.cpp" | sort)
if command -v clang-format >/dev/null; then
    clang-format --dry-run --Werror $FILES || { echo "格式不合格，请运行: clang-format -i <文件>"; exit 1; }
    echo "clang-format OK"
else
    echo "跳过（clang-format 未安装）"
fi

echo "=== 2/5 命名空间检查（octk 残留）==="
if grep -rn "octk\|OCTK_" cxxkit/ tests/ --include="*.hpp" --include="*.cpp" | grep -vE "CXXKIT|octk mechanism|from OpenCTK|Ported from|Slimmed from"; then
    echo "发现 octk 残留！"; exit 1
fi
echo "namespace OK"

echo "=== 3/5 C++11 严格性检查（PIT-22：禁 C++14 语法混入库代码）==="
if grep -rnE "std::[a-z_]+_t<|if constexpr|\[\][^)]*\(auto|0b[01]'[, ]" cxxkit/ --include="*.hpp" --include="*.cpp"; then
    echo "发现 C++14 语法残留（变量模板/_t 别名/泛型 lambda/数字分隔符）——库代码必须 C++11！"; exit 1
fi
echo "C++11 OK"

echo "=== 4/5 构建 ==="
cmake -S . -B build -G Ninja -DCXXKIT_BUILD_TESTS=ON -DCXXKIT_ENABLE_LIB_NETWORK=ON
cmake --build build --parallel 4

echo "=== 5/5 测试 ==="
ctest --test-dir build --output-on-failure
echo "=== ALL CHECKS PASSED ==="

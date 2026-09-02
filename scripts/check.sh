#!/usr/bin/env bash
# CxxKit 本地质量门禁：format + namespace + build + test + sanitizer + coverage
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== 1/8 clang-format 检查 ==="
if command -v clang-format >/dev/null; then
    CF=clang-format
elif command -v pixi >/dev/null && pixi run clang-format --version >/dev/null 2>&1; then
    CF="pixi run clang-format"
elif [ -x "$HOME/.pixi/bin/pixi" ] && "$HOME/.pixi/bin/pixi" run clang-format --version >/dev/null 2>&1; then
    CF="$HOME/.pixi/bin/pixi run clang-format"
else
    echo "[WARN] clang-format 不可用（系统未装且 pixi 环境无）——格式门禁 SKIPPED"
    echo "       安装：bash scripts/pixi-init.sh"
    CF=""
fi
if [ -n "$CF" ]; then
    # shellcheck disable=SC2086 —— $CF 故意不带引号分词
    find cxxkit tests \( -name "*.hpp" -o -name "*.cpp" \) -print0 | xargs -0 $CF --dry-run --Werror 2>&1 || { echo "格式不合格，请运行: $CF -i <文件>"; exit 1; }
    echo "clang-format OK"
fi

echo "=== 2/8 命名空间检查（octk 残留）==="
if grep -rn "octk\|OCTK_" cxxkit/ tests/ --include="*.hpp" --include="*.cpp" | grep -vE "CXXKIT|octk mechanism|from OpenCTK|Ported from|Slimmed from|WEBOCTK"; then
    echo "发现 octk 残留！"; exit 1
fi
echo "namespace OK"

echo "=== 命名一致性检查（snake 函数 + mPascal 成员）==="
SNAKE_GATE=$( { grep -rnP '\b(?!m[A-Z])[a-z]+[A-Z][a-zA-Z0-9]*\s*\(' cxxkit --include="*.hpp" --include="*.cpp" 2>/dev/null || true; } | { grep -vP ':\d+:\s*(//|\*|/\*)' || true; } | { grep -vP '\b(std::|libyuv::|[A-Z]\w+::)(if|for|while|switch|return|sizeof|catch)\b' || true; } | wc -l)
if [ "$SNAKE_GATE" != "0" ]; then
    echo "发现非 snake 函数残留（camel/Pascal 函数名）:"; grep -rnP '\b(?!m[A-Z])[a-z]+[A-Z][a-zA-Z0-9]*\s*\(' cxxkit --include="*.hpp" --include="*.cpp" 2>/dev/null | grep -vP ':\d+:\s*(//|\*|/\*)' | head -5
    exit 1
fi
MEMBER_GATE=$( { grep -rnE '^\s+.*\b[a-z][a-z0-9_]*_\s*(;|=|\{)' cxxkit --include="*.hpp" --include="*.cpp" 2>/dev/null || true; } | { grep -v preprocessor.hpp || true; } | wc -l)
if [ "$MEMBER_GATE" != "0" ]; then
    echo "发现 trailing-underscore 成员残留:"; { grep -rnE '^\s+.*\b[a-z][a-z0-9_]*_\s*(;|=|\{)' cxxkit --include="*.hpp" --include="*.cpp" 2>/dev/null || true; } | { grep -v preprocessor.hpp || true; } | head -5
    exit 1
fi
echo "naming OK"

echo "=== 3/8 C++11 严格性检查（PIT-22：禁 C++14 语法混入库代码）==="
if grep -rnE "std::[a-z_]+_t<|if constexpr|\[\][^)]*\(auto|0b[01]'[, ]" cxxkit/ --include="*.hpp" --include="*.cpp"; then
    echo "发现 C++14 语法残留（变量模板/_t 别名/泛型 lambda/数字分隔符）——库代码必须 C++11！"; exit 1
fi
echo "C++11 OK"

echo "=== 4/8 sanitizer 测试（build-asan，LSAN suppression）==="
if [ -d build-asan ]; then
    # C6：用独立 build-asan 目录（CXXKIT_BUILD_SANITIZERS=ON 生成），不动主 build
    # LSAN_OPTIONS 指向 commit 的 scripts/lsan.supp（设计进程单例，Task 4/F1 文档化）
    LSAN_OPTIONS="suppressions=$(pwd)/scripts/lsan.supp" ctest --test-dir build-asan --output-on-failure || exit 1
    echo "sanitizer OK"
else
    echo "跳过（无 build-asan；用 -DCXXKIT_BUILD_SANITIZERS=ON 生成后启用此步）"
fi

echo "=== 5/8 共享构建验证（CXXKIT_BUILD_SHARED_LIBS 动态形态; 若 build-shared 存在）==="
if [ -d build-shared ]; then
    cmake -S . -B build-shared -G Ninja -DCXXKIT_BUILD_SHARED_LIBS=ON -DCXXKIT_BUILD_TESTS=ON -DCXXKIT_ENABLE_LIB_NETWORK=OFF -DCXXKIT_ENABLE_LIB_CRASH=OFF
    cmake --build build-shared --parallel 4
    ctest --test-dir build-shared --output-on-failure || exit 1
else
    echo "跳过（无 build-shared——配置 CXXKIT_BUILD_SHARED_LIBS=ON 生成后验证动态库）"
fi

echo "=== 6/8 构建 ==="
cmake -S . -B build -G Ninja -DCXXKIT_BUILD_TESTS=ON -DCXXKIT_ENABLE_LIB_NETWORK=ON
cmake --build build --parallel 4

echo "=== 7/8 测试 ==="
ctest --test-dir build --output-on-failure

echo "=== 8/8 覆盖率报表（build-cov，R31：默认仅报表不阻断）==="
if [ -d build-cov/coverage ] && [ -f build-cov/coverage/summary.txt ]; then
    echo '--- 库源码覆盖率（build-cov/coverage/summary.txt）---'
    cat build-cov/coverage/summary.txt
    echo '--- 覆盖率报表完成（不阻断；如需强校验：CXXKIT_COVERAGE_GATE=ON 要求 ≥80%）---'
else
    echo "跳过（无 build-cov/coverage/summary.txt；用 -DCXXKIT_BUILD_COVERAGE=ON 生成后启用此步）"
fi

echo "=== ALL CHECKS PASSED ==="

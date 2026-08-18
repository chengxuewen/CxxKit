# Testing Requirements

## Minimum Test Coverage: 80%

Test Types (ALL required):
1. **Unit Tests** - Individual functions, utilities, components
2. **Integration Tests** - API endpoints, database operations
3. **E2E Tests** - Critical user flows (framework chosen per language)

## Test-Driven Development

MANDATORY workflow:
1. Write test first (RED)
2. Run test - it should FAIL
3. Write minimal implementation (GREEN)
4. Run test - it should PASS
5. Refactor (IMPROVE)
6. Verify coverage (80%+)

## Troubleshooting Test Failures

1. Use **tdd-guide** agent
2. Check test isolation
3. Verify mocks are correct
4. Fix implementation, not tests (unless tests are wrong)

## Agent Support

- **tdd-guide** - Use PROACTIVELY for new features, enforces write-tests-first

## Test Structure (AAA Pattern)

Prefer Arrange-Act-Assert structure for tests:

```cpp
TEST_CASE("cosine similarity of orthogonal vectors is zero") {
  // Arrange
  std::vector<int> a{1, 0, 0};
  std::vector<int> b{0, 1, 0};

  // Act
  double s = cosineSimilarity(a, b);

  // Assert
  REQUIRE(s == Approx(0.0));
}
```

### Test Naming

Use descriptive names that explain the behavior under test:

```cpp
TEST_CASE("returns empty vector when no items match query") {}
TEST_CASE("throws error when config key is missing") {}
TEST_CASE("falls back to substring search when index unavailable") {}
```

## 可执行检查

```bash
# 验证：测试通过
ctest --test-dir build --output-on-failure
# 验证：覆盖率（gcov / llvm-cov，按工具链选择）
ctest --test-dir build && llvm-cov gcov -r build/CMakeFiles 2>/dev/null | grep -A2 coverage
```

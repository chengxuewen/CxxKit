---
paths:
  - "**/*.cpp"
  - "**/*.hpp"
  - "**/*.cc"
  - "**/*.hh"
  - "**/*.cxx"
  - "**/*.h"
  - "**/CMakeLists.txt"
---
# C++ Coding Style

> This file extends [common/coding-style.md](../common/coding-style.md) with C++ specific content.

## Modern C++ (C++17/20/23)

- Prefer **modern C++ features** over C-style constructs
- Use `auto` when the type is obvious from context
- Use `constexpr` for compile-time constants
- Use structured bindings: `auto [key, value] = map_entry;`

## Resource Management

- **RAII everywhere** — no manual `new`/`delete`
- Use `std::unique_ptr` for exclusive ownership
- Use `std::shared_ptr` only when shared ownership is truly needed
- Use `std::make_unique` / `std::make_shared` over raw `new`

## Naming Conventions

- Types/Classes: `PascalCase` (std-mirroring containers like `flat_hash_map` and trait structs like `is_pointer` keep snake — spec §1)
- Functions/Methods: `snake_case` (post_task, to_string)
- Member variables: `mPascalCase` (mSize — encapsulated classes); POD/aggregate struct fields stay pure `snake_case`
- Static members / constants / enum values: `kPascalCase` (kInvalidHandle, kVideoRotation_0)
- Local/parameter variables: `snake_case` (pending_task)
- Namespaces: `lowercase` (new free functions go flat in `cxxkit::`)
- Macros: `UPPER_SNAKE_CASE`
- Function prefixes: bare noun = getter (size); is_/has_/should_/can_ = predicates; set_ = setter; to_ = owning conversion; as_ = non-owning view; make_ = factory; _out suffix = output param (whitelist only)

## Formatting

- Use **clang-format** — no style debates
- Run `clang-format -i <file>` before committing

## Reference

See skill: `cpp-coding-standards` for comprehensive C++ coding standards and guidelines.

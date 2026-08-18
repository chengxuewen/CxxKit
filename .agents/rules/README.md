# Rules

Rules are part of the `.agents/` directory, organized into a **common** layer plus a **cpp** layer:

```text
.agents/rules/
├── common/          # Language-agnostic principles (always install)
│   ├── coding-style.md
│   ├── testing.md
│   ├── performance.md
│   ├── patterns.md
│   ├── hooks.md
│   ├── agents.md
│   ├── code-review.md
│   ├── constraints.md
│   ├── edit-safety.md
│   ├── lesson-memory.md
│   └── security.md
└── cpp/             # C++ specific
    ├── coding-style.md
    ├── hooks.md
    ├── patterns.md
    ├── security.md
    └── testing.md
```

- **common/** contains universal principles — no language-specific code examples.
- **cpp/** extends the common rules with C++ idioms, tools, and code examples.

## Rules vs Skills

- **Rules** define standards, conventions, and checklists that apply broadly (e.g., "80% test coverage", "no hardcoded secrets").
- **Skills** (`skills/` directory) provide deep, actionable reference material for specific tasks.

## Rule Priority

When language-specific rules and common rules conflict, **language-specific rules take precedence** (specific overrides general).

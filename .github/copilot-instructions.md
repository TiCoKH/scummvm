# GitHub Copilot Custom Instructions for ScummVM Code Style

## General Style
- Language: C++ (C++11 standard for compatibility).
- Maintain cross-platform portability. Avoid platform-specific code; if unavoidable, wrap with proper `#ifdef` checks and provide fallbacks.
- Avoid exceptions — use return values, status enums, or ScummVM's error reporting utilities.
- Avoid compiler-specific extensions; stick to portable, standard-compliant code.

## File Structure
1. At the top, include the license and copyright notice.
2. Use `#pragma once` for header guards.
3. Includes go after the guard: first the matching header, then other project headers, then system headers.
4. Namespaces should be clearly opened/closed.

## Naming Conventions
- **Classes**: PascalCase (e.g., `MyClass`).
- **Methods**: camelCase starting lowercase (`doSomething`).
- **Member Variables**: prefix with `_` (`_myMember`).
- **Constants/Enum entries**: ALL_CAPS with underscores (`MAX_VALUE`).
- Avoid obscure abbreviations.

## Class and Method Organization
- Order: **public** then **protected** then **private**.
- Constructors/destructors first in each section.
- In `.cpp`, implement functions in the same order as in `.h`.

## Formatting
- Indentation: 4 spaces, no tabs.
- Opening brace on the same line for methods, control structures.
- Max line length: ~80 characters when possible.
- One declaration per line.

## Code Practices
- Use ScummVM's built-in types (like `Common::String`, `Common::List`) instead of STL where applicable.
- Avoid magic numbers; use constants or enums.
- Minimize global variables.
- Prefer references when null is not meaningful.

## Comments
- Use `//` for single line, `/* */` for multi-line.
- Document purpose of classes/methods and tricky logic.

By applying these rules, your generated code will be more consistent with the ScummVM project’s standards.

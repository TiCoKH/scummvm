# GitHub Copilot Custom Instructions for ScummVM Code Style and Formatting

## General Code Style
- Use standard C++11 for portability.
- Avoid platform-specific or compiler-specific extensions when possible.
- Do not use C++ exceptions; use return codes, status enums, or ScummVM's error handling methods.
- Prefer ScummVM's built-in types over STL containers (e.g., `Common::String` instead of `std::string`).
- Keep savegame and API backward compatibility unless explicitly approved to break it.

## File Structure
1. License header at the top of every file.
2. Use traditional include guards in the format `PATH_FILENAME_H` derived from the relative path in the source tree. All uppercase letters and directory separators replaced by underscores.
3. Include files in this order:
   - Corresponding header file.
   - ScummVM project headers.
   - Third-party library headers.
   - System headers.
4. Group includes logically and avoid unused ones.

## Naming Conventions
- **Classes**: PascalCase (`MyClass`).
- **Methods**: camelCase starting lowercase (`doSomething`).
- **Member Variables**: prefix with `_` (`_myValue`).
- **Constants, Macros, Enums**: ALL_CAPS (`MAX_VALUE`).
- Avoid unclear abbreviations.

## Class and Method Organization
- Order: **public**, then **protected**, then **private**.
- Constructors/Destructors first in each section.
- Inline only very short and performance-critical functions.
- Implement `.cpp` methods in the same order as the `.h`.

## Formatting Rules (from ScummVM Code Formatting Conventions)
- Indentation: 4 spaces, no tabs.
- One statement per line.
- Opening brace `{` on the same line for method definitions and control statements.
- Closing brace `}` aligned with the start of the definition/statement.
- Place `else` on the same line as the closing brace of the preceding `if` block.
- No space before a semicolon.
- Keep lines at or under 80 characters where reasonable.
- Add a single space after commas in parameter lists.
- No space before opening parenthesis in function calls (`func(x, y)`).
- Always use a space between control keywords and parentheses: `if (x)`.
- Never have multiple empty lines in a row.
- Align continued lines by indenting at least one level.

## Comments
- Use `//` for single-line, `/* ... */` for multiple lines.
- Document complex algorithms, assumptions, and non-obvious code.
- File-level and class-level descriptions are required.

## Error Handling
- No C++ exceptions.
- Use ScummVM's error reporting through return values, enums, or logging helpers.
- Check input validity explicitly.

## Portability and Safety
- Avoid relying on unspecified sizes of primitive types; use fixed-size types (e.g., `uint32`).
- Minimize usage of `static` globals.
- All platform-dependent code must be isolated and guarded with preprocessor macros.

By following these instructions, Copilot will generate code consistent with ScummVM's standards for formatting, style, and portability.

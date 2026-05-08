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

## Event System Usage Guide

### View vs Dialog Event Handling
- **View**: Single instance, managed by view stack, uses msgFocus/msgUnfocus
- **Dialog**: Multiple instances can be visible simultaneously, uses activate/deactivate

### View Constructor (loadtime - runs once)
✓ Create member pointers, empty UI elements, parse static data
✗ Don't access game state, VmInterface, or assume visibility

### msgFocus (view gains focus via replaceView/addView)
✓ Get game state/party, activate/deactivate UI, refresh data
✗ Don't create dialogs or init pointers (constructor's job)

### msgUnfocus (view loses focus)
✓ Save changes, cleanup temp state, stop timers
✗ Don't delete children or block the unfocus

### Dialog Constructor (loadtime - runs once)
✓ Create child dialogs/menus, init member pointers, configure UI elements
✗ Don't access game state, VmInterface, or assume visibility

### activate() (dialog becomes active/visible)
✓ Get game state/character data, refresh data, activate children
✗ Don't create child dialogs (constructor's job)

### deactivate() (dialog becomes inactive/hidden)
✓ Save changes, cleanup temp state, deactivate children
✗ Don't delete children

### msgKeypress (keyboard input)
✓ Forward to child dialogs first, return true if handled, call redraw()
✗ Don't handle all keys - return false for unhandled

### msgMouseMove/Enter/Leave (mouse hover)
✓ Update hover states, tooltips, cursor appearance
✗ Don't trigger actions (MouseDown/Up) or expensive ops (called often)

### msgMouseDown/Up (clicks)
✓ Use MouseUp for confirmation, check active/enabled, return true if handled
✗ Don't trigger on MouseDown only or skip bounds checks

### draw() (every frame)
✓ Draw static then child dialogs, keep idempotent, mark dirty with redraw()
✗ Don't modify game state or create UI (constructor) or load data (msgFocus/activate)

### Event Flow: User Action → Events routes to focused view → msgXXX → forward to children → redraw()

## Multi-Dialog Management in Views

When a View needs to manage multiple dialogs (only one visible at a time):

### Helper Methods
- `attachDialog(dialog)` - Add dialog to view hierarchy and activate it
- `detachDialog(dialog)` - Deactivate dialog and remove from view hierarchy
- `switchDialog(oldDialog, newDialog)` - Detach old, attach new in one call

### Pattern
1. **Constructor**: Create all dialogs, call `subView()` only for always-visible ones
2. **Stage switching**: Use `attachDialog()`/`detachDialog()` to show/hide dialogs
3. **Destructor**: Delete all dialogs (no need to detach first)

### Example
```cpp
// Constructor - create but don't attach
_itemsMenu = new ItemsMenu();

// Switch to items stage
attachDialog(_itemsMenu);
setActiveSubView(_itemsMenu);

// Switch back to profile stage
detachDialog(_itemsMenu);
setActiveSubView(_profileDialog);
```

## ECL VM Architecture (Goldbox Engine)

### Single Operand Access Path (Runtime Path B)
The ECL VM uses a **single operand access path**: all operand data is read at runtime directly from VM flat memory via `EclVM::getOperand(N)`, `readVar(i)`, `readString(i)`, and `getOpWord(i)`.

- `EclInstruction` is a **lightweight boundary record** containing only `pc` and `opcode`. It does NOT store operand data.
- `decodeProgram()` scans bytecode to find instruction boundaries (skipping operand bytes) but does NOT decode or store operands.
- Opcode handlers access operands exclusively through `EclVM` methods that parse from flat memory at the current PC.

### Opcode Handler Signature
```cpp
typedef int (*OpcodeHandler)(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack,
        SyscallHandler *syscalls);
```
- No `EclInstruction` parameter — handlers never receive pre-decoded operand data.
- Handlers call `vm.getOperand(N)` to parse N operands from the bytecode stream at the current PC.
- Use `vm.readVar(i)` for dereferenced numeric values, `vm.getOpWord(i)` for raw addresses, `vm.readString(i)` for strings.

### Varargs Opcodes (ON GOTO, ON GOSUB, VERTICAL MENU, HORIZONTAL MENU)
For variable-argument opcodes:
1. First call `vm.getOperand(fixedCount)` to read the count operand.
2. Then call `vm.getOperand(fixedCount + count)` to re-decode all operands including varargs.
3. Access varargs via `vm.getOpWord(fixedCount + 1 + index)`.

### Parameter Roles
- **`vm`** — Operand decoding only. Call `vm.getOperand(N)` to read N operands from the bytecode stream, then `vm.readVar(i)` for resolved values, `vm.getOpWord(i)` for raw addresses, `vm.readString(i)` for strings. Never call `vm.step()` or touch `vm._pc` directly.
- **`mem`** — Flat 64K VM address space. All game state lives here. Use `getOpcodeLayout()` to resolve named fields to VM addresses.
- **`nextPc`** — Next instruction PC, passed by reference. Already set to the default next instruction. Overwrite it for branches (`GOTO`, `GOSUB`, `RETURN`, `IF` skip). Do not set it to the current instruction's PC.
- **`callStack`** — GOSUB/RETURN return-address stack. `GOSUB` pushes `nextPc` then overwrites it; `RETURN` pops into `nextPc`.
- **`syscalls`** — Engine-side callbacks (rendering, menus, combat, sound). Always guard with `if (!syscalls) return VM_ERROR` before use.

### Return Values
- `VM_OK` — continue to `nextPc`
- `VM_HALTED` — script ended (`EXIT`, `STOP MOVE`)
- `VM_ERROR` — unrecoverable fault (null syscalls, empty call stack, divide by zero)
- `VM_YIELD` — reserved for future async suspension; do not use in handlers

### Key Rules
- NEVER store operand data in `EclInstruction` or any pre-decoded structure.
- NEVER pass `EclInstruction` to opcode handlers.
- ALWAYS use `vm.getOperand()` / `vm.readVar()` / `vm.readString()` for operand access.
- The decoder exists solely to build a PC→instruction-index mapping for `findInstructionIndexByPc()`.

### Rules
✓ Call `vm.getOperand(N)` before any `readVar`/`getOpWord`/`readString` call
✓ Use `setCmpResult(mem, ...)` for all compare-style opcodes (IF opcodes read from it)
✓ Guard syscall use with `if (!syscalls) return VM_ERROR`
✓ Return `VM_OK` for stubs/no-ops rather than leaving them unregistered
✗ Don't modify `vm._pc` directly — only `nextPc`
✗ Don't skip registering a handler — unregistered opcodes return `VM_ERROR` at runtime
✗ Don't add entries to `opcode_table.cpp` without a matching registered handler

### Operand Count Source of Truth
The handler's `vm.getOperand(N)` call determines the actual operand count consumed. The `opcode_table.cpp` entry is metadata for the decoder/disassembler and must match.

## MenuResult Event System

Child dialogs communicate results to parent views/dialogs via event-based messaging:

### Posting Results (Child → Parent)
✓ Use `g_events->postMenuResult()` to send results to parent
✓ Always pass parent's `getName()` as target
✓ Deactivate child before posting result
✗ Don't call parent methods directly or assume parent lifetime

### Handling Results (Parent)
✓ Override `handleMenuResult(const MenuResultMessage &result)` to receive results
✓ Extract values using `result._hasIntValue`, `result._hasStringValue` flags
✓ Decide whether to keep/deactivate/delete child based on result
✗ Don't assume child still exists after handling result

### MenuResultMessage Fields
- `_targetViewName` - Parent view/dialog name (use `getName()`)
- `_success` - Operation succeeded (true) or cancelled (false)
- `_keyCode` - Key that triggered result (e.g., KEYCODE_RETURN, shortcut key)
- `_intValue` / `_hasIntValue` - Optional integer value (e.g., menu selection index)
- `_stringValue` / `_hasStringValue` - Optional string value (e.g., text input)

### Example
```cpp
// Child posts result
deactivate();
if (_parent)
    g_events->postMenuResult(_parent->getName(), true,
        Common::KEYCODE_RETURN, selectedIndex,
        Common::String(), true, false);

// Parent handles result
void ParentView::handleMenuResult(const MenuResultMessage &result) {
    if (!result._success) {
        // User cancelled
        return;
    }
    if (result._hasIntValue) {
        int selection = result._intValue;
        // Process selection
    }
}
```
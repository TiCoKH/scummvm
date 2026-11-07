# GitHub Copilot Instructions for ScummVM

Read [AI-GUIDELINES.md](../AI-GUIDELINES.md) in full before making changes.

The rules in AI-GUIDELINES.md are mandatory and override anything below. In short:
follow project formatting/naming/commit conventions, keep comments short (no
lengthy LLM-style explanations), keep everything GPLv3+ compatible, never
generate art or media assets, and add an attribution trailer to commits:

```
Assisted-by: <agent>:<model-id>
```

## General Code Style
- Standard C++11 for portability; avoid platform- or compiler-specific extensions.
- No C++ exceptions. Use return codes, status enums, or ScummVM error helpers
  (`error()`, `warning()`, `debug()`).
- Prefer ScummVM types over STL (`Common::String`, `Common::Array`, ...).
- Use fixed-size types (`uint8`, `uint16`, `uint32`, ...) where size matters.
- Minimize `static` globals. Isolate platform-dependent code behind preprocessor guards.
- Keep savegame and API backward compatibility unless explicitly approved to break it.
- Check input validity explicitly.

## File Structure
1. GPL license header at the top of every file.
2. Traditional include guards named from the path relative to `engines/`,
   uppercase, separators replaced by `_` (e.g. `GOLDBOX_ECL_ECL_ENGINE_HOST_H`).
3. Include order: corresponding header, ScummVM headers, third-party headers,
   system headers. No unused includes.

## Naming
- Classes: PascalCase (`MyClass`).
- Methods: camelCase (`doSomething`).
- Member variables: `_` prefix (`_myValue`). Exception: plain data records that
  mirror original game structs (characters, items, spells, effects) keep public
  fields without `_` (`pc->healthStatus`).
- New constants and enum values: `kName` (`kPlayfieldCols`).
- ALL_CAPS only for macros and for enums that mirror original game data or
  Pascal names (`S_OKAY`, `GS_SHOP`, `SP_MUL2_KNOCK`, `VM_OK`).
- Avoid unclear abbreviations.

## Class Organization
- Order sections public, protected, private; constructors/destructor first.
- Inline only very short, performance-critical functions.
- Implement `.cpp` methods in the same order as declared in the `.h`.

## Formatting
- Indent with tabs, align with spaces (`.editorconfig`: `indent_style = tab`,
  width 4). New files use tabs. Older goldbox files use 4 spaces; when editing
  one, follow its existing style and don't reindent unrelated lines.
- One statement per line.
- `{` on the same line for functions and control statements; `}` aligned with
  the start of the statement; `} else {` on one line.
- `if (x)` - space after control keywords; `func(x, y)` - no space before the
  call paren, one space after commas, no space before `;`.
- Keep lines around 80 columns where reasonable; indent continuation lines at
  least one level.
- Never more than one consecutive empty line.

## Comments
- `//` for single lines, `/* ... */` for blocks, `/** ... */` for doc comments.
- Document complex algorithms, assumptions and non-obvious code only. Keep it brief.
- No planning/ticket IDs in code comments (`GB13-009 (8.1)`, `task 8.3`); state
  the reason itself. Referencing original-game offsets or Pascal routine names is fine.

## Gold Box UI: Views, Dialogs and Events

### View vs Dialog
- `VIEW_` / View: single instance managed by the view stack; a larger gameplay
  interaction container that coordinates dialogs and their lifecycle. Uses
  `msgFocus` / `msgUnfocus`.
- `DIALOG_` / Dialog: a partial interaction unit owning part of the screen and
  its input. Not necessarily a modal box. Can nest inside a view or another
  dialog. Uses `activate()` / `deactivate()`.
- Input goes to the deepest active dialog first; unhandled input bubbles to the
  parent dialog, then to the containing view.
- Dialogs may call gameplay systems for authoritative state changes, but
  gameplay rules belong in system/domain classes, not dialogs.
  - No dice rolls, odds tables or value tables in dialogs/views. A dialog asks
    a rules function for the outcome (`DoorRules::tryBash(party, doorFlag)`,
    `TreasureRules::rollGemValue()`) and only presents it.
  - Rule code must not depend on UI classes, so it stays reusable by the
    other Gold Box games and testable without a screen.

### Porting Legacy Code
- Preserve the original interaction state machine.
- Split at meaningful interaction boundaries (distinct input, selection or
  prompt loops), not because a function is large or branchy.
- Do not map one legacy function to one dialog class mechanically.

### Lifecycle
| Hook | Do | Don't |
|---|---|---|
| View constructor | create member pointers, empty UI elements, parse static data | access game state / VmInterface, assume visibility |
| `msgFocus` | read game state/party, (de)activate UI, refresh data | create dialogs or init pointers |
| `msgUnfocus` | save changes, clean temp state, stop timers | delete children, block the unfocus |
| Dialog constructor | create child dialogs/menus, init pointers, configure UI | access game state / VmInterface, assume visibility |
| `activate()` | read game state/character data, refresh, activate children | create child dialogs |
| `deactivate()` | save changes, clean temp state, deactivate children | delete children |
| `msgKeypress` | forward to children first, return true if handled, `redraw()` | swallow all keys; return false when unhandled |
| `msgMouseMove/Enter/Leave` | hover state, tooltips, cursor | trigger actions or expensive work |
| `msgMouseDown/Up` | confirm on MouseUp, check active/enabled and bounds, return true if handled | act on MouseDown only |
| `draw()` | static content then children, idempotent, `redraw()` to mark dirty | modify game state, create UI, load data |

Flow: user action -> Events routes to focused view -> `msgXXX` -> children -> `redraw()`.

### Multiple Dialogs in a View (one visible at a time)
- `attachDialog(d)` adds to the hierarchy and activates; `detachDialog(d)`
  deactivates and removes; `switchDialog(old, new)` does both.
- Constructor: create all dialogs, `subView()` only the always-visible ones.
- Destructor: delete all dialogs (no detach needed).

```cpp
_itemsMenu = new ItemsMenu();      // constructor: create, don't attach

attachDialog(_itemsMenu);          // switch to items stage
setActiveSubView(_itemsMenu);

detachDialog(_itemsMenu);          // back to profile stage
setActiveSubView(_profileDialog);
```

### MenuResult (child -> parent)
- Child: `deactivate()` first, then `g_events->postMenuResult()` with the
  parent's `getName()` as target. Never call parent methods directly or assume
  the parent's lifetime.
- Parent: override `handleMenuResult(const MenuResultMessage &result)`, check
  `_hasIntValue` / `_hasStringValue`, then decide whether to keep, deactivate or
  delete the child. Don't assume the child still exists afterwards.
- Fields: `_targetViewName`, `_success` (false = cancelled), `_keyCode`,
  `_intValue`/`_hasIntValue`, `_stringValue`/`_hasStringValue`.

```cpp
deactivate();
if (_parent)
    g_events->postMenuResult(_parent->getName(), true,
        Common::KEYCODE_RETURN, selectedIndex,
        Common::String(), true, false);

void ParentView::handleMenuResult(const MenuResultMessage &result) {
    if (!result._success)
        return;
    if (result._hasIntValue) {
        int selection = result._intValue;
        // ...
    }
}
```

## Gold Box ECL VM

### Operand Access
Operands are decoded lazily from VM flat memory while the handler runs. There
is no pre-decoded program or instruction record; don't add one.

- `vm.getOperand(N)` decodes N operands at the current PC and advances the
  VM's next-instruction PC past them. Call it before any `readVar(i)`
  (resolved value), `getOpWord(i)` (raw address) or `readString(i)`.
- The handler's `getOperand(N)` is the source of truth for operand count; the
  `opcode_table.cpp` metadata must match it.

### Handler Signature
```cpp
typedef int (*OpcodeHandler)(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack,
        SyscallHandler *syscalls);
```
- `vm`: operand decoding only. Never call `vm.step()` or touch `vm._pc`.
- `mem`: flat 64K address space holding all game state. Resolve named fields
  with `getOpcodeLayout()`.
- `nextPc`: leave it alone for normal flow; the VM advances past the operands.
  Overwrite it only for branches (`GOTO`, `GOSUB`, `RETURN`, `IF` skip).
- `callStack`: `GOSUB` pushes `nextPc` then overwrites it; `RETURN` pops into `nextPc`.
- `syscalls`: engine callbacks. Guard with `if (!syscalls) return VM_ERROR;`.

### Varargs Opcodes (ON GOTO, ON GOSUB, VERTICAL/HORIZONTAL MENU)
1. `vm.getOperand(fixedCount)` to read the count.
2. `vm.getOperand(fixedCount + count)` to re-decode everything.
3. Access varargs with `vm.getOpWord(fixedCount + 1 + index)`.

### Return Values
- `VM_OK` continue at `nextPc`; also use for stubs/no-ops.
- `VM_YIELD` an async host operation started (menu, input, print pacing,
  delay, shop, combat). The VM suspends with its resume PC recorded and the
  host resumes it through the `PendingOperationStack`.
- `VM_HALTED` script ended (`EXIT`, `STOP MOVE`).
- `VM_ERROR` unrecoverable (null syscalls, empty call stack, divide by zero).

### Async Opcodes
- The host's `begin*Async()` returns `VM_YIELD`, `VM_ERROR` or `VM_OK`
  (= not supported, use the synchronous fallback). Handlers return
  `VM_YIELD`/`VM_ERROR` unchanged and undo any temporary state they set.
- On `VM_YIELD`, any post-operation tail must run when the operation
  completes, before the resume PC is fetched. Never re-execute the opcode
  and also apply its effect.
- Don't block or spin inside a handler waiting for input; yield instead.

### Rules
- Use `setCmpResult(mem, ...)` for all compare-style opcodes; IF opcodes read it.
- Register a handler for every opcode (unregistered ones return `VM_ERROR`).
- Every `opcode_table.cpp` entry needs a matching registered handler.
- New host features go on the host interface as virtual methods with a safe
  default, not as new `dynamic_cast<EclEngineHost *>(syscalls)` sites in handlers.

### Core vs Dialect
- Core (`ecl_vm` and common handlers): interpreter mechanics (`GOTO/GOSUB/RETURN`,
  compare+branch, memory read/write, math, operand decode) and default opcode semantics.
- Per-game host callbacks (`SyscallHandler` / `EclEngineHost`): resource and UI
  behavior (DAX loading, character/monster struct adaptation, dialogs/input).
- If only resource integration differs, reuse the core handler and call
  game-specific host methods; don't fork it.
- If a game truly redefines an opcode byte, override only that opcode's metadata
  and/or handler in the dialect. Prefer small delta overlays over copied tables.
- Precedence for both metadata and handlers: dialect override -> core fallback.

## Poolrad Legacy Save Compatibility (Do Not Regress)

- Keep the legacy DAT layout exact:
  - 1-byte lead value `3`.
  - VMBANK order/sizes: `0x4900/0x0800`, `0x6B00/0x0800`, `0x9700/0x0400`, `0x9900/0x1E00`.
  - Tail size `0x150`: `posX, posY, dir, mapType, gameState, charCount, charTable`.
- Character table entries are Pascal records (len + data, max `0x28`).
- ScummVM-created legacy saves name companions `CHRDAT<slot><n>.SAV/.ITM/.SPC`
  (slot F: `CHRDATF1.SAV`).
- The loader must accept base names from the table (`CHRDATA<n>` and
  `CHRDAT<slot><n>`) and extension case variants (`.ITM/.itm`, `.SPC/.spc`).
- `.SAV` is required; `.ITM/.SPC` are optional. Don't write zero-length
  `.ITM/.SPC` when there are no items/effects.
- For transfer-style loads (`gameState == GS_START_MENU`), skip geo/icon/wall
  preloads in `loadGameSlotX86` so restored banks aren't mutated before re-save.
- Legacy spell arrays: memorized spells must not auto-set known spells on load.
- Pointer-field differences (e.g. `.SPC` next pointers) are acceptable as long
  as gameplay and cross-platform load compatibility are preserved.

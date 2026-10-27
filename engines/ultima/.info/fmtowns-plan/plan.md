# Ultima I FM Towns: EGB/BIOS-subset-first implementation plan

Date: 2026-10-27

Status: DRAFT, revised to the user's API-first scope. Implement graphics and
text first; keyboard, timing and audio/CD follow. Missing intro function bodies
block sequence reconstruction, not the documented graphics/text subset.

## Summary and scope

Implement an engine-local behavioral subset of the EGB graphics/text calls used
by Ultima I, with documented state, parameter and result semantics. Then add the
other required BIOS service subsets, and use UT1DEMO as the first application
consumer. DARKNESS reuses the same API. Do not
execute the original executable, emulate DOS, or introduce a ScummVM platform
backend. Preserve existing DOS and enhanced rendering.

The user confirmed Ultima I as the first target and priority/transparent-pixel
overlay rather than alpha blending. The requested visible output is 640x480.
The supplied manuals now document priority and the meaning of display-start
modes. Overlay transparency, hardware-specific origin constraints and the exact
resolution-code configuration still require verification.

This is a source-driven draft, not an approved migration specification. No
separate feature spec, constitution, topology, or API contract was supplied.
The requirement IDs below organize the user request and discovered integration
needs; they are not claims of independently approved requirements.

Deliverable boundaries:

- First: a documented and validated EGB graphics/text subset, independently
  callable without an intro scene, assets, audio, or recovered `main_loop`.
- Next: required keyboard/timing and audio/CD services mapped to ScummVM.
- Then: a verified static intro scene and the complete intro sequence,
  skip/completion behavior, and audio cues once missing evidence is recovered.
- Then: prove API reuse with a DARKNESS screen and bitmap/text operations.
- Deferred: full FM Towns gameplay, save-format conversion, other Ultima games,
  complete EGB/BIOS emulation, and unobserved graphics modes.

## Technical context and constraints

- Language/framework: existing ScummVM C++ engine and message-driven views.
- Graphics: `Graphics::ManagedSurface`, existing Ultima `Screen`, palette and
  font facilities; a single final output surface.
- Storage: externally supplied original game assets; no database or network.
- Timing/input/audio: ScummVM event clock, input events, mixer/available CD
  audio facilities, not original interrupts or busy-wait loops.
- Initial representation: two indexed planes with explicit per-plane palettes.
  Expand packed 4-bit asset pixels to one byte per index internally.
- Output decision: prefer a combined indexed palette for confirmed two-16-color
  operation; reserve separate palette ranges for each plane. Use RGB only if
  actual used modes require it. The intro's 16-bit helper declarations alone do
  not justify implementing a true-color path.
- Keep visible output dimensions separate from plane storage dimensions.
  Do not assume that all original draw coordinates are within the viewport.
- No engine code, builds, or runtime validation were performed for this plan.
  Analysis covered the graphics wrappers, initialization, asset transfers,
  text/window operations, and relevant current integration; this is not an
  exhaustive audit of the main game's gameplay logic.

## Source analysis

### Supplied documentation and newly resolved semantics

Local manuals are in `D:\RevEng\FMTowns`. They remain external references;
do not copy their prose, examples or scans into source or redistribute them.
The following facts are paraphrased from the named function sections of
`C Library V2.1 Reference.docx`, cross-checked where noted against
`TOWNS BIOS Reference.docx`. DOCX paragraph indices below are zero-based XML
paragraph positions for locating this edition, not printed page numbers.

| Contract | Documentation locator | Planning consequence |
|---|---|---|
| `EGB_displayPage(work, pri, display)`: priority 0 puts page 0 on top; priority 1 puts page 1 on top. Display values 0/1/2/3 mean neither/page 0/page 1/both. | C reference, `EGB_displayPage`, paragraphs 1313-1338; BIOS function 06H | The DARKNESS wrapper's mapping is now interpretable: wrapper 0/1/2/3 means page 0/page 1/both/neither, with priority 0. Keep game wrapper translation separate from EGB semantics. |
| `EGB_displayStart` mode 0 sets screen display-start position; 1 moves within the virtual screen; 2 sets enlargement; 3 sets display size. Bit 6 suppresses the VSYNC wait. | C reference, `EGB_displayStart`, paragraphs 1395-1449 | The decompiled constant 1 is a mode, not a page selector. It must not be interpreted as "always operate on page 1." Recover page/state selection and mode-specific constraints from the full contract. |
| Write modes 0, 6 and 9 are PSET, MATTE and OPAQUE respectively. MATTE skips the transparent source color; OPAQUE draws pattern-zero positions with the background color. | C reference, `EGB_writeMode`, paragraphs 4434-4497 | Implement these documented modes first. MATTE drawing transparency is not automatically the same as display-plane overlay transparency. |
| Paint mode separates boundary, surface-fill and background pattern/solid controls; `0x22` is a documented example. | C reference, `EGB_paintMode`, paragraphs 2704-2738 | Decode fields explicitly. `34` is `0x22`; `1058` is `0x422`. Read remaining field details before assigning full behavior to either value. |
| Write-page selection also selects the active 16-color palette; page-size changes can affect the viewport. | BIOS function 05H, paragraphs 702-705 | Switching write page has documented state consequences beyond changing a surface pointer. |

These are targeted reads, not a completed review of every EGB section.
Verify translation/OCR ambiguities and diagram-dependent details against the
Japanese HTML/scans in `D:\RevEng\FMTowns\FM TOWNS Bios Reference`.

### First milestone: call-level subset

Use the intersection of documented contracts and observed game call sites.
Declarations without executable call evidence remain candidates. Preserve EGB
operation boundaries and semantics, but use typed C++ state/surfaces instead of
real-mode registers, segmented pointers, or a binary-compatible work buffer.

| Group | Initial subset / candidates | Order |
|---|---|---|
| State and display | `init`, `resolution`, `writePage`, `displayPage`, `displayStart`, `viewport`, `palette`, `color`, `writeMode`, `paintMode`, `clearScreen` | First |
| Primitives | `pset`, `connect`, `rectangle`, `circle`, `arc`, `fan`; `penSize` where confirmed | After state/raster semantics |
| Bitmap operations | `putBlock`, `partScroll`; `getBlock` and `putBlockZoom` only when actual use is confirmed | After raster semantics; memory buffers suffice, no TIFF dependency |
| Text | `fontStyle`, `textDirection`, `textDisplayDirection`, `textSpace`, `asciiString`, `sjisString` | First milestone, after drawing state; only used styles/directions required |
| Non-EGB BIOS/services | Confirm actual KYB, timing, font-service and sound/CD entry points and distinguish BIOS calls from library/game helpers | Subsequent milestone; keyboard/timing before audio/CD |

The subset needs a per-call contract ledger: manual section, observed caller,
parameter ranges/layout, state read/written, clipping rules, supported modes,
return/error behavior, and conformance checks. Keep this ledger in this plan
as it is completed. No unsupported call may silently act like a supported one.
If font lookup needs another BIOS service, implement only that dependency in
the graphics/text milestone rather than pulling in unrelated BIOS emulation.

### Evidence levels

- **Observed**: a function body in the supplied export shows the operation.
- **Declared only**: a name/prototype appears, but its implementation is absent.
- **Proposed**: an implementation choice, not a recovered hardware fact.
- **Unresolved**: requires original binary disassembly, documentation, assets,
  or a reference run.

Register-derived pseudo-arguments, inferred pointer types, guessed globals, and
IDA-assigned names are not reliable ABI specifications. The supplied header is
useful as a decoding aid, not a set of structs to import into the engine.

### UT1DEMO: what is actually available

| Evidence | Source | Consequence |
|---|---|---|
| Declared only: initialization, page setup, slide loading/drawing, keyboard waits, `main_loop`, and `main` | [declarations](../UT1DEMO_Headerless.c#L73) | Cannot recover slide order, durations, complete setup, or termination semantics from these declarations. |
| Observed: display-start mode 1 receives `(0,0)` or `(54,49)`; another wrapper adds a horizontal parameter to 54 | [sub_8C4E8 and sub_8C524](../UT1DEMO_Headerless.c#L1246) | Documented virtual-screen movement, independent of drawing and rectangle scrolling. Hardware-specific units/constraints/wrap require the full mode contract. |
| Observed: point, rectangle, circle, temporary paint-mode change | [graphics wrappers](../UT1DEMO_Headerless.c#L1264) | Small primitive/state API is appropriate; the reused parameter storage is not one universal rectangle structure. |
| Observed: box-sized clear changes box state and drawing colors | [sub_8D614](../UT1DEMO_Headerless.c#L1308) | Box/window policy belongs above low-level plane composition. |
| Observed: a text wrapper advances horizontal indentation by two and wraps/scrolls against box limits | [sub_8DB90](../UT1DEMO_Headerless.c#L1332) | Preserve explicit text cursor/cell state; glyph encoding and width semantics need more evidence. |
| Declared only: palettes, 4-bit/16-bit color helpers, write modes, block capture/blit/zoom, SJIS/ASCII, arc/fan and partial scroll | [API declarations](../UT1DEMO_Headerless.c#L15) | An API inventory, not evidence that every operation is executed by the intro. |
| Observed: CD-related and FM-sound helper bodies exist; high-level playback cue bodies are largely absent | [CD helper](../UT1DEMO_Headerless.c#L823), [sound helper](../UT1DEMO_Headerless.c#L1229) | Audio abstraction is needed; intro track/cue mapping is not yet recoverable. |
| Header layouts describe B/R/G palette tones, signed block coordinates, arc/fan fields and packed box fields | [local types](../UT1DEMO_Headerless.h#L12) | Normalize colors/types at an asset or behavior boundary; verify layout against actual stores. |

Important limitation: the current FM Towns demo C++ file loads named slides,
but its DOS-like logo/castle/trademark sequence is not original-executable
evidence. Do not treat it as the recovered UT1DEMO timeline.

### DARKNESS: concrete reusable behavior

| Evidence | Source | API implication |
|---|---|---|
| Original wrapper maps input 0/1/2/3 to EGB values 1/2/3/0 with priority 0 | [GFX_setDisplayPage](../DARKNESS_Headerless.c#L124727) | Manual confirms page 0/page 1/both/neither, with page 0 above page 1. Keep wrapper values distinct from EGB display bits. |
| Both saved resolution fields become 3; write mode is 9; paint mode is 34; text directions and spacing are zeroed | [GFX_Init_Darkness](../DARKNESS_Headerless.c#L124764) | Explicit deterministic state reset. The decompiler omits `EGB_resolution` arguments, so code 3 does not independently prove storage dimensions. |
| Setup hides display, selects plane 1, sets its palette/origin, then selects plane 0 and draws frame/header/windows before showing both via wrapper value 2 | [InitialSettings](../DARKNESS_Headerless.c#L135005) | Write selection, visibility, origin and palette are distinct operations. Avoid presenting partial setup. |
| Origin wrappers use `(54,49)` and `(a1+54,17)` | [origin wrappers](../DARKNESS_Headerless.c#L124742) | Origin is two-dimensional. Do not model all movement as one shared horizontal scroll remainder. |
| Temporary write modes 0 and 9; mode 6 for a small bitmap operation | [rectangle helper](../DARKNESS_Headerless.c#L124968), [header text](../DARKNESS_Headerless.c#L125411), [sub_CAD80](../DARKNESS_Headerless.c#L126043) | Manual identifies PSET, OPAQUE and MATTE. Implement each contract and restoration; do not confuse MATTE with XOR or display transparency. |
| Paint mode is masked with `0xFF0F`, ORed with `0x20`, restored, and elsewhere set to 1058 | [primitive wrappers](../DARKNESS_Headerless.c#L124852), [another paint use](../DARKNESS_Headerless.c#L125900) | Decode only used modes; simple fill-versus-outline guesses are insufficient. |
| Block destinations use inclusive endpoints; one 32x32 tile advances source by 512 bytes | [block wrappers](../DARKNESS_Headerless.c#L125124) | Strong evidence for packed 4-bit tile storage. Convert inclusive coordinates to half-open ScummVM rectangles at one boundary. |
| Loader reads 512 header bytes and transfers the remaining payload; row size depends on bit depth | [TIFF_LoadTileSet and TIFF_loadHEADTTL](../DARKNESS_Headerless.c#L125184) | Verify the actual game TIFF variant before choosing standard decoding or a narrowly scoped custom decoder. |
| Loader has branches for 1, 4, 8 and 16 bits, including an unusual 1-bit row-width treatment | [row sizing](../DARKNESS_Headerless.c#L125259) | Branch presence is not proof of asset use or standard TIFF semantics. Verify actual assets and packing. |
| Window interiors scroll vertically by -16 using partial scroll | [TXT_scroll](../DARKNESS_Headerless.c#L125503) | Rectangle pixel scrolling is distinct from changing a plane's display origin; exposed-pixel fill and overlap behavior matter. |
| Text uses an 8-pixel cell advance, 16-pixel row spacing, and SJIS lead/trail checks | [TXT_outSjis](../DARKNESS_Headerless.c#L125564) | Need Japanese-aware wrapping; byte count, glyph count, and display cells are not interchangeable. Verify baseline semantics of EGB text positions. |
| Text selection can advance past one NUL-terminated string before displaying the next | [TXT_outString](../DARKNESS_Headerless.c#L124600) | Resource parsing must preserve alternate strings; exact language selector meaning requires confirmation. |
| Frame draws arcs and fans around 16-pixel-radius corners | [GFX_DrawGameScreen](../DARKNESS_Headerless.c#L125001) | Implement observed arc/fan semantics, not just arbitrary quarter-circle substitution. |
| Main map buffer is assembled from packed tiles; plane 1 palette entries are changed without rebuilding all pixels | [map transfer and palette changes](../DARKNESS_Headerless.c#L126433) | Palette changes must invalidate output independently of pixel writes. |
| Some destination extents exceed 640, e.g. x=382 with width=304 | [title animation](../DARKNESS_Headerless.c#L135120) | Determine clipping/storage/origin behavior; do not resize the visible screen or silently discard data based on a guessed model. |

### Current implementation gaps to address, not build-verified findings

- [TownsScreen](../../shared/gfx/towns_screen.cpp#L33): uninitialized plane
  state and `_system`, incomplete pixel accessor, clear that does not clear
  pixels, undeclared-in-body arc variables, absent `update()`/fan definitions,
  and inclusive/exclusive dirty-rectangle mismatches.
- [TownsScreen header](../../shared/gfx/towns_screen.h#L118): extra class
  qualification on private declarations; drawing/window responsibilities mixed
  with composition.
- [Engine lifetime/setup](../../shared/early/ultima_early.cpp#L47):
  `_townsScreen` is not initialized in the constructor, including failure paths;
  renderer ownership and destruction ordering need an explicit contract.
- [Frame update](../../shared/engine/events.cpp#L146) currently updates the
  ordinary screen, not the plane compositor.
- [View surface](../../shared/gfx/visual_item.cpp#L55) always comes from the
  ordinary screen; Towns views must target a plane intentionally.
- [Ultima I game setup](../../ultima1/game.cpp#L62) assigns null FM Towns
  views, then startup calls through `_gameView`; `"Demo"` also differs from the
  demo constructor's `"Intro"` name.
- [Demo implementation](../../ultima1/u1gfx/fmt/view_towns_demo.cpp#L22)
  includes a nonexistent-looking `view_demo.h` and implements `ViewDemo`,
  while [its header](../../ultima1/u1gfx/fmt/view_towns_demo.h#L39) declares
  `ViewTownsDemo`. Resolve this specific build surface, not unrelated views.
- [module.mk](../../module.mk#L54) lists `towns_window.o` although the
  available window definition is header-only; Towns view objects are not listed.
  Reconcile the selected build's actual sources.
- [TIFF loader](../../shared/gfx/tiff_loader.cpp#L33) only returns pixels, not
  palette/format metadata. [Custom decoding](../../shared/gfx/tiff.cpp#L66)
  needs an asset-backed correctness review before reuse.
- [TownsWindow](../../shared/core/towns_window.h#L40) leaves cursor/state fields
  uninitialized and uses a 6-pixel capacity calculation, whereas the observed
  main text path advances by 8. Recover semantics before reconciling these.

## Requirements

| ID | Requirement |
|---|---|
| REQ-001 | Keep a source/confidence inventory and gate unresolved original behavior. |
| REQ-002 | Render two independently configured planes into a 640x480 priority-overlay output. |
| REQ-003 | Separate plane selection, visibility, palette, display origin, clipping and raster state. |
| REQ-004 | Load and transfer verified original bitmap assets with correct packing and coordinates. |
| REQ-005 | Provide the primitives and write/paint operations actually used by targeted scenes. |
| REQ-006 | Preserve window, text, Japanese encoding and rectangular-scroll behavior. |
| REQ-007 | Recreate UT1DEMO as a responsive scene sequence with verified input/audio behavior. |
| REQ-008 | Integrate the renderer and intro safely into Ultima I without changing other platforms. |
| REQ-009 | Demonstrate reuse of the graphics API with a representative DARKNESS scene. |
| REQ-010 | Validate measurable rendering results and document remaining fidelity gaps. |

## Proposed API shape

This table is a draft responsibility map, not a stable ABI or generated C++.
Expose the selected EGB operations with recognizable names and documented
semantics within the existing `TownsScreen` implementation where appropriate.
Do not replace them with scene-specific shortcuts. A separate facade is only
justified if implementation demonstrates a real ownership/state boundary.
Do not reproduce segmented pointers, registers or the physical work-area ABI.

| Owner | Proposed operations | Contract |
|---|---|---|
| Shared `TownsScreen` | configure plane; select write plane; enable display planes; set display origin | Explicit per-plane state. Internal visibility takes a mask; any original 0..3 wrapper translation is separate. Priority is configured only according to verified behavior. |
| Shared `TownsScreen` | set palette range; set transparent index; clear plane | Palette updates invalidate output. Clearing changes stored indices. Do not assume EGB's transparent drawing color is also the hardware overlay key. |
| Shared `TownsScreen` | set clip; set supported raster/paint state; point/line/rectangle/arc/fan | Clip in plane coordinates; restore temporary state. Unsupported modes report an explicit diagnostic rather than behaving like copy mode. |
| Shared `TownsScreen` | blit; capture; scaled blit; scroll rectangle | Use typed surfaces/source rectangles, explicit pitches, overlap-safe scroll/copy and verified exposed-area fill. Add capture/zoom only when a recovered scene needs them. |
| Shared `TownsScreen` | compose into output | Read both planes without modifying them; resolve origins and priority, map final indices/colors, invalidate affected output. No backend presentation inside setters. |
| Shared `TownsWindow` plus Ultima I presentation | configure window; reset cursor; write encoded text; newline; clear/hide/show | Window policy above graphics. Convert resources to a deliberate encoding representation, preserving SJIS character boundaries and original cell widths. |
| Ultima I `ViewTownsDemo` | enter scene; advance by deadline; accept input; finish/cancel | Own timeline and asset references, not renderer ownership. No original executable launch. |
| Existing engine audio integration | start/stop verified cue or track | ScummVM playback and settings; no BIOS/interrupt reproduction. No silent substitution for an unimplemented required cue. |

Begin with only confirmed two-16-color operation. Internal 8-bit storage permits
convenient surface operations, but does not imply support for 256-color source
modes. Define plane background and both-disabled output explicitly. Determine
whether transparent indices affect only the top plane or other cases from a
reference run, not from assumptions about black.

## Unknowns and release gates

| Gate | Missing evidence / decision | Blocks |
|---|---|---|
| G0 | Recover UT1DEMO `main`, `main_loop`, `InitialSettings`, `GFX_Init_U1Intro`, page setup, slide load/draw, key wait, header/box drawing and high-level audio bodies, with referenced strings/data | Faithful scene sequence only; does not block documented API implementation |
| G1 | Complete manual-backed resolution/storage/origin constraints and overlay transparent-key behavior; priority and display-mask meaning are resolved | Hardware-faithful compositor/origin behavior |
| G2 | Complete PSET/MATTE/OPAQUE details, paint fields/masks, palette color order and primitive endpoints; write-mode names are resolved | Accurate text/blits/primitives |
| G3 | Inspect real TIFF headers/payloads, palette source, nibble order, image orientation, and required font source/metrics | Accurate static scene and text |
| G4 | Establish original intro completion/skip destination and resource/audio ownership reset | Intro-to-main handoff |

Prefer a fuller export of the missing functions and relevant disassembly;
unreliable inferred prototypes alone are insufficient. Reference screenshots or
video should include language/configuration and a timing basis. Screenshots can
settle geometry/colors but cannot alone settle packed data, origin units, or
raster operations. A synthetic compositor milestone can proceed with explicitly
provisional settings; it must not be described as original-game parity.

## Implementation steps and task breakdown

Tasks are for human-reviewed implementation, not instructions to copy the
decompilation. All source paths are repository-relative. Dependencies are
explicit; task numbering alone does not imply prerequisites.

### Phase 1 / Plan 1.1: define the documented graphics/text subset

Requirements: REQ-001, REQ-003, REQ-007.

Resolve each gate only as far as the next milestone requires. Record recovered
addresses, actual parameters, assets, observed output and confidence in this
plan. Keep unresolved behavior visible.

- [ ] T001 [Plan:1.1] Complete the per-call EGB graphics/text contract ledger here using `D:\RevEng\FMTowns\C Library V2.1 Reference.docx`, `D:\RevEng\FMTowns\TOWNS BIOS Reference.docx` and the observed call sites in both exports; distinguish confirmed calls from declaration-only candidates. [Source: engines/ultima/.info/UT1DEMO_Headerless.c] [Source: engines/ultima/.info/DARKNESS_Headerless.c]
- [ ] T002 [Plan:1.1] Resolve remaining G1/G2 details from the manuals and cited wrappers in `engines\ultima\.info\DARKNESS_Headerless.c`; record supported modes, result/error contracts and state effects here, using disassembly/reference runs for actual-game configuration rather than substituting guessed prototypes. [Source: engines/ultima/.info/DARKNESS_Headerless.c#GFX_setDisplayPage,GFX_Init_Darkness,InitialSettings]
- [ ] T003 [Plan:1.1] Inspect privately supplied images/fonts and record the actual asset manifest, format, dimensions, palette and text metrics here; complete G3 without committing assets or generated art.

Dependencies: T001/T002 can proceed from the supplied manuals and exports.
T003 is an independent asset-fidelity investigation; missing assets do not
block memory-buffer drawing or a documented text contract. Missing intro
sequence bodies are deferred to Phase 5, not a prerequisite for this milestone.

### Phase 2 / Plan 2.1: deterministic graphics foundation

Requirements: REQ-002, REQ-003, REQ-008.

Complete two-plane state and composition using the actual surface format and
pitch. Keep the existing screen as the output/presentation owner. For combined
indexed output, assign disjoint palette ranges and compare transparent source
indices before mapping. Palette changes, display-origin changes and visibility
changes require recomposition even if no plane pixels changed.

- [ ] T004 [Plan:2.1] Complete deterministic plane/state initialization, explicit storage format/pitch, real clear, checked pixel access, lifetime and declaration fixes in `engines\ultima\shared\gfx\towns_screen.h` and `engines\ultima\shared\gfx\towns_screen.cpp`; initialize `_townsScreen` on all engine paths in `engines\ultima\shared\early\ultima_early.cpp`.
- [ ] T005 [Plan:2.1] Implement full-frame composition, independent palettes, confirmed transparency/priority, origins and output clipping in `engines\ultima\shared\gfx\towns_screen.cpp`; distinguish internal display mask from original wrapper values. [Source: engines/ultima/.info/DARKNESS_Headerless.c#GFX_setDisplayPage,GFX_DisplayStart,InitialSettings] [Source: engines/ultima/.info/UT1DEMO_Headerless.c#sub_8C4E8,sub_8C524]
- [ ] T006 [Plan:2.1] Wire one frame path in `engines\ultima\shared\gfx\screen.h`, `engines\ultima\shared\gfx\screen.cpp`, `engines\ultima\shared\early\ultima_early.cpp` and, only if needed, `engines\ultima\shared\engine\events.cpp`: draw views, compose, retain cursor behavior, then present once; keep non-Towns operation unchanged.

Dependencies: T004 after T002's state decisions; T005 after T004 and G1;
T006 after T005. Start with full-frame redraw, not the incomplete dirty list.
Expose a pure composition boundary that does not require locking backend VRAM.

### Phase 3 / Plan 3.1: EGB raster, primitives and block operations

Requirements: REQ-004, REQ-005.

Prefer existing compatible image facilities. Repair the current custom TIFF
path only where the actual game format requires it; do not add another decoder
without proving a gap. Preserve indices/palette metadata through decoding.

- [ ] T007 [Plan:3.1] Resolve the verified TIFF variant and palette retention in `engines\ultima\shared\gfx\tiff_loader.h`, `engines\ultima\shared\gfx\tiff_loader.cpp`, `engines\ultima\shared\gfx\tiff.h` and `engines\ultima\shared\gfx\tiff.cpp`, or route this loader through an existing compatible decoder; reject truncated/unsupported input with resource-specific diagnostics. [Source: engines/ultima/.info/DARKNESS_Headerless.c#TIFF_LoadTileSet,TIFF_loadHEADTTL]
- [ ] T008 [Plan:3.1] Implement documented `putBlock` semantics and required packed-tile conversion in `engines\ultima\shared\gfx\towns_screen.h` and `engines\ultima\shared\gfx\towns_screen.cpp` using memory-buffer inputs; convert inclusive endpoints once, preserve EGB clipping/state, and use real pitches. Do not require the TIFF loader to exercise this API. [Source: engines/ultima/.info/DARKNESS_Headerless.c#sub_C99EC,sub_C9A4C]
- [ ] T009 [Plan:3.1] Implement only verified write/paint modes and required points/lines/rectangles/arcs/fans in `engines\ultima\shared\gfx\towns_screen.cpp`; preserve temporary state and signed axes. Defer capture/zoom unless a recovered scene uses them. [Source: engines/ultima/.info/UT1DEMO_Headerless.c#sub_8C738,sub_8C7B4,sub_8C7F1,sub_8C824] [Source: engines/ultima/.info/DARKNESS_Headerless.c#GFX_DrawGameScreen,sub_C95EC,sub_CAD80]

Dependencies: T007 after T003, as an asset-adapter task that may be completed
after the graphics/text API. T009 after T004 and relevant G2 contracts;
T008 after T009's raster behavior. T008/T009 touch the same files: serialize.

### Phase 4 / Plan 4.1: text and window presentation

Requirements: REQ-005, REQ-006.

Reuse ScummVM's existing FM Towns/Japanese font facilities if their metrics and
licensing match. If they do not, document the precise gap before extending font
support. Do not substitute DOS 8x8 output for final FM Towns text.

- [ ] T010 [Plan:4.1] Complete initialized window/cursor/style state and recovered geometry in `engines\ultima\shared\core\towns_window.h` and its implementation if needed; reconcile capacity units with actual call sites, not the current 6-pixel assumption. [Source: engines/ultima/.info/UT1DEMO_Headerless.h#U1_box_data] [Source: engines/ultima/.info/DARKNESS_Headerless.c#SCR_loadBoxConfig,TXT_setFontStyle12p]
- [ ] T011 [Plan:4.1] Add encoding-aware text output, measured glyph/cell metrics, newline/wrap, and alternate-string resource selection using `engines\ultima\shared\gfx\font.h`, `engines\ultima\shared\gfx\font.cpp`, `engines\ultima\shared\early\game.cpp` and `engines\ultima\shared\core\towns_window.h` as appropriate; retain non-Towns font behavior. [Source: engines/ultima/.info/DARKNESS_Headerless.c#TXT_outString,TXT_outSjis] [Source: engines/ultima/.info/UT1DEMO_Headerless.c#sub_8DB90]
- [ ] T012 [Plan:4.1] Implement overlap-safe rectangular pixel scrolling and window clear/hide/show composition in `engines\ultima\shared\gfx\towns_screen.cpp` and the window implementation; keep it separate from display-origin scrolling. [Source: engines/ultima/.info/DARKNESS_Headerless.c#TXT_scroll,TXT_setupScroll,sub_CA2D4]

Dependencies: T010 after relevant T002 decisions; T011 after T009, T010 and
the documented font-source/metric contract (real-asset fidelity uses T003).
T012 after T008 and T010. Do not schedule shared-file edits in parallel.
A window implementation can remain header-only if that is simplest.

**First-milestone exit gate:** after T006/T008/T009/T011/T012, validate each
selected EGB call against its contract with controlled memory inputs and exact
pixel/state expectations. Validate invalid inputs and unsupported modes too.
Separate EGB text rendering from higher-level window wrapping. Record exact
checks/results in this plan before implementing keyboard/timing/audio or
reconstructing the intro. Run the applicable checks from T018 at this gate;
do not wait until a complete scene exists to discover API semantic errors.

### Phase 5 / Plan 5.1: remaining BIOS services, then UT1DEMO integration

Requirements: REQ-007, REQ-008.

After the graphics/text exit gate, inventory the actual keyboard/timing and
audio/CD calls, distinguish BIOS entries from C-library/game helpers, and
document their parameters, state and results here. Map the required behavior
to existing ScummVM facilities; do not emulate interrupt vectors or port I/O.
Complete keyboard/timing before audio/CD, without inventing original cues.
Then draw one source-verified scene using both planes and implement the
recovered sequence as a small deadline-driven state machine; introduce a scene
table only if the recovered sequence benefits. Restore/reset graphics and audio
state at handoff rather than inheriting accidental executable-process state.

- [ ] T013 [Plan:5.1] Reconcile `ViewTownsDemo` header/class/include/name and register its source in `engines\ultima\ultima1\u1gfx\fmt\view_towns_demo.h`, `engines\ultima\ultima1\u1gfx\fmt\view_towns_demo.cpp` and `engines\ultima\module.mk`; replace the DOS-derived placeholder content with one verified FM Towns scene using the plane API. [Source: engines/ultima/.info/UT1DEMO_Headerless.c]
- [ ] T014 [Plan:5.1] Recover the missing intro sequence/input/timing bodies and document the used service contracts here; implement keyboard/timing behavior through existing engine events/clock facilities, then slide order, deadlines and interruption policy in `engines\ultima\ultima1\u1gfx\fmt\view_towns_demo.cpp`; handle quit/return-to-launcher and release owned surfaces deterministically. [Source: engines/ultima/.info/UT1DEMO_Headerless.c#main_loop,InitialSettings,GFX_drawSlide,KEYB_waitKeyPress]
- [ ] T015 [Plan:5.1] Map recovered intro cues to ScummVM audio integration from `engines\ultima\ultima1\u1gfx\fmt\view_towns_demo.cpp` and `engines\ultima\ultima1\game.cpp`; preserve audio settings and stop cues on skip/exit/handoff. Missing required media or unsupported cues must be reported explicitly. [Source: engines/ultima/.info/UT1DEMO_Headerless.c#CDPlayer_playTrack]
- [ ] T016 [Plan:5.1] Construct/register valid Towns views and route completion/skip/load entry in `engines\ultima\ultima1\game.h`, `engines\ultima\ultima1\game.cpp` and `engines\ultima\ultima1\u1gfx\fmt\view_towns_title.cpp`; eliminate null-view entry and inconsistent view names, reset renderer state, and gate all changes by FM Towns platform.

Dependencies: T013 after T007, the graphics/text exit gate and any
T009/T011/T012 operations needed by
the selected scene. A diagnostic static scene does not require the full timeline.
T014's service work follows the graphics/text exit gate; its sequence work
follows T013 and recovery of G0. T015 after the cue inventory recovered in
Phase 5 and T014's keyboard/timing service work.
T016 after T014/T015/T017 and G4, using the valid title/representative main
view as its destination. Declared-only source anchors in T014/T015 must
be replaced with real recovered evidence before those tasks begin.

### Phase 6 / Plan 6.1: prove DARKNESS reuse without porting the whole game

Requirements: REQ-004, REQ-005, REQ-006, REQ-009.

- [ ] T017 [Plan:6.1] Build a representative main/title presentation in `engines\ultima\ultima1\u1gfx\fmt\view_towns_title.h` and `engines\ultima\ultima1\u1gfx\fmt\view_towns_title.cpp` using the same API: frame/header, plane-1 bitmap, text window, palette change, and partial text scroll; reproduce the verified initialization/reset sequence. [Source: engines/ultima/.info/DARKNESS_Headerless.c#InitialSettings,GFX_DrawGameScreen,TXT_scroll,UT1_loadEngineData]

Dependencies: T017 after T006/T008/T009/T011/T012 and G1/G2/G3.
It can precede full intro sequencing if G0 remains blocked. Do not use the
incomplete Towns title's DOS timing or palette helpers as source authority.

### Phase 7 / Plan 7.1: validate, then optimize and document

Requirements: REQ-002, REQ-008, REQ-010.

- [ ] T018 [Plan:7.1] Capture the acceptance evidence below for `engines\ultima\shared\gfx\towns_screen.cpp` and both Towns views; run the configured Ultima I build and applicable existing tests, compare reference scenes, and record exact commands/results and unresolved gates in this plan.
- [ ] T019 [Plan:7.1] Only after full-frame results are correct, repair/enable dirty composition in `engines\ultima\shared\gfx\towns_screen.cpp` if profiling warrants it; map plane damage through origins, recompose both planes in damaged output, and invalidate on palette/visibility/origin changes.
- [ ] T020 [Plan:7.1] Recheck DOS/enhanced behavior, finalize build/source declarations in `engines\ultima\module.mk` and capability needs in `engines\ultima\configure.engine`, and update this plan with the implemented API, supported modes, validation commands/results and remaining unsupported gameplay.

Dependencies: T018's API checks run at the Phase 4 exit gate; its scene checks
follow T013 and T017. Full intro acceptance also requires
T014/T015/T016. T019 after a passing full-frame baseline in T018; if no
optimization is necessary, record it as intentionally omitted. T020 after T018
and the T019 decision, and after T016 for completion of this scope.

## Project structure

Prefer changes to existing graphics, view, engine and resource files. No new
backend, executable interpreter, or standalone EGB compatibility library.
If a separate window implementation is justified, use
`engines\ultima\shared\core\towns_window.cpp` and reconcile its existing object
entry. Keep Ultima I sequence policy in its `u1gfx\fmt` views rather than in the
shared compositor. Keep original executables/assets out of implementation files.

## Testing strategy and acceptance

This is a plan, not a test execution report. No new test files are generated.
The existing documented unit command is `make test` in
[test/README](../../../../test/README); no Ultima-named test files were found
by the targeted filename search. This does not prove there is no indirect
graphics coverage. Inspect relevant graphics tests before implementation.
No tests or builds were run for this documentation-only task.

- **appType:** native desktop game engine; no HTML, HTTP, database or services.
- **primaryValidationStack:** configured Ultima I C++ build, applicable existing
  CxxTest tests, deterministic surface inspection, and native runtime comparison
  with the original release/reference environment.
- **infra-tier/browser-tier:** not applicable. Docker, Node and Playwright are
  not prerequisites for this feature; no availability-based fallback is chosen.
- **environment requirements:** repository-supported native build toolchain and
  backend, Ultima I enabled (it is disabled by default), matching `ultima.dat`,
  legally supplied FM Towns data, fonts/audio, and original/reference captures.
  Select exact build commands from the developer's configured toolchain; do not
  claim an unexecuted Windows build command is validated.
- **dependency strategy:** compose and inspect memory surfaces without backend
  VRAM; load real private assets for format checks, never fabricated substitutes
  for fidelity acceptance. Use normal engine input/clock/audio boundaries.
- **test infrastructure:** reuse existing CxxTest surface/palette fixtures where
  suitable. Any separately requested automated cases should use that runner,
  not a new test framework; shared setup must initialize all plane/window state.
- **legacyTestAssets:** no user-supplied tests; no dedicated Ultima test file
  found by the filename scan. Reuse existing relevant tests, do not discard them.
- **test data:** keep original assets read-only; use isolated settings/save
  locations. Do not overwrite the user's saves. Keep reference captures private
  unless redistribution is permitted.
- **knownGaps/fallbacks:** without assets, only synthetic composition is
  verifiable; without a reference run, hardware fidelity is unverified; without
  recovered intro bodies, sequence/audio/input parity is blocked. None is a
  passing substitute for original-game parity.

### Critical journeys and measurable acceptance

1. **Compose and alter planes.** Output is exactly 640x480. Check source indices
   and expected final colors for transparent foreground, opaque black, same
   index/different palette colors, each visibility combination, and both planes
   disabled. Palette-only changes update every affected pixel.
2. **Change origins and mutate content.** Confirm direction/units/wrap against
   reference; clears/moving foreground reveal current background, never stale
   pixels. Right/bottom pixels `(639,479)` are handled correctly. Storage clipping
   is evaluated separately from visible output clipping.
3. **Load/draw a real intro scene.** Verify image dimensions/orientation,
   4-bit nibble order (including odd widths), palette order, clipping, primitive
   geometry and pixel-aligned composition against a named reference scene.
   Record exact pixel differences where practical, rather than "looks right."
4. **Text/window operations.** Match ASCII/Japanese glyphs, baseline and cell
   spacing; no split SJIS characters at wrapping boundaries. A -16 scroll moves
   the intended interior exactly one observed row, leaves borders unchanged,
   and fills the newly exposed area correctly.
5. **Play/interrupt/complete the intro.** Match recovered scene ordering and
   timestamped cue/transition targets; select and record tolerances from the
   reference timing basis before acceptance. Quit/skip are handled by the next
   normal event-processing opportunity, including during waits. Audio stops or
   continues only as verified; handoff starts from defined graphics state.
6. **Enter DARKNESS and preserve other platforms.** Representative title/main
   scene reuses the compositor with no duplicated plane logic. DOS 320x200 and
   enhanced views retain their existing layouts, palettes, cursors and startup.
   Test direct load separately; unsupported Towns gameplay must remain explicit,
   not a null dereference or success-shaped fallback.

Each implementation task must record its smallest applicable build/check and
exact result; report blocked/unrun checks explicitly. A nonzero build/test exit
code is failure, even if some checks pass. No completion claim while a required
journey or semantic gate is unresolved. The human reviewer checks asset
provenance, mode evidence, command outcomes, screenshots/pixel comparisons,
timing evidence, regressions and any intentionally deferred optimization.

## Requirement mapping

Evidence below names planned implementation surfaces, not completed work.

| REQ ID | Description | Plan items | Planned implementation evidence |
|---|---|---|---|
| REQ-001 | Source/confidence inventory | 1.1 | This plan's evidence and gate tables; recovered exports |
| REQ-002 | Two-plane output | 2.1, 7.1 | `shared\gfx\towns_screen.cpp`; pixel comparisons |
| REQ-003 | Independent graphics state | 1.1, 2.1 | `shared\gfx\towns_screen.h/.cpp`; verified mode/origin mapping |
| REQ-004 | Asset decoding/transfers | 3.1, 6.1 | `shared\gfx\tiff_loader.cpp`, `shared\gfx\towns_screen.cpp`; real assets |
| REQ-005 | Observed primitives/raster operations | 3.1, 4.1, 6.1 | `shared\gfx\towns_screen.cpp`; reference frame/text behavior |
| REQ-006 | Text/windows/partial scroll | 4.1, 6.1 | `shared\core\towns_window.h`, `shared\gfx\font.cpp`; glyph/scroll evidence |
| REQ-007 | Responsive original intro | 1.1, 5.1 | `ultima1\u1gfx\fmt\view_towns_demo.cpp`; recovered timeline/cues |
| REQ-008 | Safe platform-gated integration | 2.1, 5.1, 7.1 | `shared\early\ultima_early.cpp`, `shared\gfx\screen.cpp`, `ultima1\game.cpp`, `module.mk` |
| REQ-009 | Main-game reuse proof | 6.1 | `ultima1\u1gfx\fmt\view_towns_title.cpp`; representative DARKNESS scene |
| REQ-010 | Verified results and gaps | 7.1 | Exact build/check outcomes, reference comparisons and resolved/remaining gates here |

Coverage: 10 derived requirements, 7 plan items, 20 tasks. Coverage means that
work is planned, not that it is implemented or original-game behavior verified.

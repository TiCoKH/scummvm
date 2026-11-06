/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Assisted-by: Claude:claude-opus-4.6
 *
 */

#ifndef GOLDBOX_POOLRAD_NATIVE_SAVE_H
#define GOLDBOX_POOLRAD_NATIVE_SAVE_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/error.h"
#include "common/serializer.h"
#include "common/stream.h"
#include "common/str.h"

#include "goldbox/core/global.h"
#include "goldbox/poolrad/location_context.h"

namespace Goldbox {
namespace Poolrad {

class PoolradEngine;

namespace Data {
class PoolradCharacter;
}

/**
 * GB13-033 subtasks 24.2 + 24.3 — versioned native save schema, serializer and
 * transactional load.
 *
 * 24.2 replaced the GB13-033 placeholder (a single dummy uint32) with a real,
 * versioned schema that round-trips the authoritative campaign state through
 * ScummVM stream/Serializer facilities only. 24.3 made the LOAD side
 * transactional (readInto -> validate -> commit) so a malformed/truncated/
 * wrong-profile/missing-resource load leaves the previous game fully usable,
 * and enabled native save (Engine::kNativeSaveImplemented) now that load is
 * safe and save is bounded to stable map states. It serializes NO raw pointers,
 * GUI/view/dialog objects, DAX/tile/picture/sprite caches, mixer handles or
 * surfaces: those are derived state rebuilt from the schema (24.3).
 *
 * Section order (single Serializer pass, load and save share one path):
 *   1. header/profile   magic + version + gameId + platform + features + state
 *   2. party            count, per-character payload (via PoolradCharacter
 *                       save/load), selection index
 *   3. LocationContext  kind/region/ids/coords/dir/indoorRender (explicit; the
 *                       kind has no VM field so it cannot be rebuilt from VM
 *                       memory alone)
 *   4. VM memory image  AddressSpace::MEMORY_SIZE bytes (world/quest flags,
 *                       clock, script/resource ids, dungeon/wilderness pos,
 *                       search/preparation bytes all live here)
 *   5. RNG seed         Common::RandomSource::getSeed()
 *
 * The clock fields (Poolrad 0x49C5..0x49CC) live INSIDE the VM memory image,
 * so they are covered by section 4 and are deliberately NOT double-encoded.
 *
 * OPEN items carried from 24.2 into 24.3, each documented at its use site:
 *   - RNG live state: Common::RandomSource exposes only getSeed()/setSeed()
 *     (the INITIAL seed, not the live internal MT state). We serialize the
 *     seed; exact post-reload determinism parity is an OPEN caveat.
 *   - _targetCharacter / _nextCharacter are NOT round-tripped (only the active
 *     selection is). They are transient command/combat targets; a stable map
 *     boundary never has one pending. Documented OPEN.
 *   - VM call-stack / PC resume: we TIGHTEN the save boundary to stable map
 *     states instead of serializing the VM call stack (see isSaveBoundaryStable).
 */
namespace NativeSave {

/** 8-byte on-disk tag. Distinguishes a native save from the old dummy payload. */
extern const char kNativeSaveMagic[8]; // "GBXNSV00"
const byte kNativeSaveMagicSize = 8;

/**
 * Schema version. Bump when the layout changes; every optional field is read
 * behind a minVersion so 24.3 (and later) can reject or migrate old saves.
 */
const Common::Serializer::Version kNativeSaveVersion = 1;

/** Sentinel written in the selection slot when no character is selected. */
const uint16 kNoSelectionIndex = 0xFFFF;

/**
 * Pure, headless-testable save-boundary inputs (Requirement 30.3).
 *
 * A save is only allowed at a stable boundary: a plain dungeon/wilderness map
 * state, the VM not suspended mid-script, and no unresolved host request /
 * half-applied transaction. Combat, camping/rest, shop and the menus are
 * unsupported for native save and must be DISABLED there, not advertised.
 *
 * Kept as a plain struct of fed-in values so the predicate can be unit-tested
 * without constructing a live PoolradEngine.
 */
struct SaveBoundaryState {
	GameState gameState;       ///< current engine game state
	bool vmSuspended;          ///< true while the VM is yielded mid-script
	bool hasPendingHostRequest;///< unresolved async host op / pending operation
	bool hasPendingTransaction;///< half-applied transaction in progress

	SaveBoundaryState()
		: gameState(GS_START_MENU), vmSuspended(false),
		  hasPendingHostRequest(false), hasPendingTransaction(false) {}
};

/**
 * True only when the current context supports a clean native save:
 *   - gameState is GS_DUNGEON_MAP or GS_WILDERNESS_MAP (the only stable map
 *     states; combat/camping/shop/menus/after-combat/end-game are excluded,
 *     chosen conservatively so the VM call stack is irrelevant);
 *   - the VM is not suspended mid-script;
 *   - no unresolved host request and no half-applied transaction.
 */
bool isSaveBoundaryStable(const SaveBoundaryState &state);

/**
 * GB13-033 24.3 — parsed, owning temporary for a transactional native load.
 *
 * readInto() fills a LoadedState from the stream WITHOUT touching the live
 * engine. validate() then checks it against the running engine, and only on
 * success does commit() swap it into the engine. A failure at any earlier step
 * leaves the previous game fully usable: the LoadedState is simply discarded
 * and its owned PoolradCharacter allocations are freed in the destructor.
 *
 * The struct OWNS everything it holds. On commit() the party characters are
 * transferred to the engine and @c party is cleared so the destructor does not
 * free them; before commit (and on every error path) the destructor frees
 * them, so there is no leak and no double-free.
 */
struct LoadedState {
	// Section 1: profile. Captured verbatim; validate() compares these against
	// the running engine (wrong-profile rejection, Requirement 30.2).
	Common::String profileGameId;
	uint32         profilePlatform;
	uint32         profileFeatures;
	uint32         profileGameState;

	// Section 2: party. Freshly constructed characters in party order plus the
	// active-selection index (kNoSelectionIndex when none). Owning pointers.
	Common::Array<::Goldbox::Poolrad::Data::PoolradCharacter *> party;
	uint16 selectionIndex;

	// Section 3: location context (plain data).
	LocationContext location;

	// Section 4: VM memory image. Owned heap buffer; NOT written to live memory
	// until commit(). @c vmImageLength is the length read from the stream so
	// validate() can reject a size mismatch before the buffer is applied.
	byte  *vmImage;
	uint32 vmImageLength;

	// Section 5: RNG seed (seed-level only; see OPEN note on live-state parity).
	uint32 rngSeed;

	LoadedState()
		: profileGameId(), profilePlatform(0), profileFeatures(0),
		  profileGameState(0), party(), selectionIndex(kNoSelectionIndex),
		  location(), vmImage(nullptr), vmImageLength(0), rngSeed(0) {}

	~LoadedState() { reset(); }

	/** Free every owned allocation and clear the buffers. Safe to call twice. */
	void reset();

private:
	// Non-copyable: it owns raw PoolradCharacter allocations and a heap buffer.
	LoadedState(const LoadedState &);
	LoadedState &operator=(const LoadedState &);
};

/**
 * Transactional load, step 1 — parse the ENTIRE stream into @p out without
 * touching the live engine. Validates magic/version first (rejecting the old
 * dummy payload and unknown/newer versions). A stream truncation or read error
 * at ANY point returns a clear Common::Error; @p out is left owning only what
 * it parsed and is safely discarded by the caller (its destructor frees it),
 * so the live engine is never half-mutated.
 *
 * Takes the raw stream (not a Serializer) so end-of-stream can be detected:
 * a Serializer hides the stream and a short read on a MemoryReadStream does not
 * set err(), only eos(). The engine is passed const for profile capture only.
 */
Common::Error readInto(Common::SeekableReadStream &stream, LoadedState &out);

/**
 * Transactional load, step 2 — validate a parsed LoadedState against the
 * running @p engine. Returns a distinct, truthful Common::Error per failure
 * class (Requirement 30.2): wrong gameId/profile, bad party count, out-of-range
 * selection, VM image size mismatch, and unresolved resource references
 * (mapId/ECL/GEO ids that do not map to real DAX blocks). Does not mutate
 * anything.
 */
Common::Error validate(const LoadedState &in, const PoolradEngine &engine);

/**
 * Transactional load, step 3 — swap a validated LoadedState into the live
 * @p engine. ONLY call after readInto() and validate() both succeed. Frees the
 * old party, moves the parsed characters in (transferring ownership, so @p in
 * no longer frees them), applies the selection by index, writes the validated
 * VM image, sets the LocationContext and RNG seed, then transitions the game
 * state to the resolved map state so the engine rebuilds derived/view/cache
 * state (ECL/GEO/viewport), mirroring loadGameSlotX86's final setGameState.
 */
void commit(LoadedState &in, PoolradEngine &engine);

/**
 * Single SAVE entry (mirrors Engine::syncGame's one-method pattern). On SAVE it
 * writes the full schema from @p engine's live state. On LOAD it is now
 * TRANSACTIONAL: it reads the whole stream into a temporary LoadedState
 * (readInto), validates it (validate) and only then swaps it in (commit), so a
 * malformed/truncated/wrong-profile/missing-resource load returns the first
 * error with the previous game fully intact (Requirement 30.2, GB13-033 24.3).
 *
 * The magic/version check and the old-dummy-payload rejection happen inside
 * readInto on load and inline here on save.
 */
Common::Error sync(Common::Serializer &s, PoolradEngine &engine);

/**
 * Transactional native LOAD entry (GB13-033 24.3). Drives readInto -> validate
 * -> commit over the raw stream so end-of-stream truncation is detectable (a
 * Serializer hides the stream). Returns the first error with the previous game
 * fully intact on failure; on success the engine holds the loaded campaign.
 * PoolradEngine::loadGameStream routes here instead of through the Serializer
 * seam used for saving.
 */
Common::Error loadNative(Common::SeekableReadStream &stream, PoolradEngine &engine);

} // namespace NativeSave

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_NATIVE_SAVE_H

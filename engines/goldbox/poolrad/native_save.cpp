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

#include "goldbox/poolrad/native_save.h"

#include "common/array.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/span.h"
#include "common/textconsole.h"

#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"

namespace Goldbox {
namespace Poolrad {
namespace NativeSave {

// "GBXNSV00": Goldbox native save, format generation 00. A native save always
// starts with these 8 bytes; the old GB13-033 dummy payload (a bare uint32)
// never does, so a stale/placeholder stream is rejected at the magic check.
const char kNativeSaveMagic[8] = { 'G', 'B', 'X', 'N', 'S', 'V', '0', '0' };

bool isSaveBoundaryStable(const SaveBoundaryState &state) {
	// Requirement 30.3: only the plain, stable map states support a clean
	// native save. Everything else (combat, camping/rest, shop, start/menu,
	// after-combat, end-game) is excluded conservatively. This choice keeps
	// the VM call stack irrelevant to the save: a stable map boundary is never
	// mid-script, so there is no call frame to serialize or restore.
	if (state.gameState != GS_DUNGEON_MAP &&
	    state.gameState != GS_WILDERNESS_MAP)
		return false;

	// A VM suspended mid-script has a half-evaluated continuation that the
	// schema does not capture; reject rather than save a torn state.
	if (state.vmSuspended)
		return false;

	// An unresolved async host request (menu/shop/combat/delay/print) or a
	// half-applied transaction means live state is in flux.
	if (state.hasPendingHostRequest || state.hasPendingTransaction)
		return false;

	return true;
}

void LoadedState::reset() {
	for (uint i = 0; i < party.size(); ++i)
		delete party[i];
	party.clear();
	selectionIndex = kNoSelectionIndex;

	delete[] vmImage;
	vmImage = nullptr;
	vmImageLength = 0;
}

// ---------------------------------------------------------------------------
// SAVE side. Writes the live engine state in the versioned section order. This
// is unchanged from 24.2 in encoding; the LOAD side is what became
// transactional in 24.3 (readInto/validate/commit below).
// ---------------------------------------------------------------------------

namespace {

// Section 1: header/profile. Game/release/platform identity + active state.
void saveProfile(Common::Serializer &s, PoolradEngine &engine) {
	Common::String gameId = engine.getGameId();
	s.syncString(gameId);

	uint32 platform = static_cast<uint32>(engine.getPlatform());
	s.syncAsUint32LE(platform);

	uint32 features = engine.getFeatures();
	s.syncAsUint32LE(features);

	uint32 gameState = static_cast<uint32>(engine.getGameState());
	s.syncAsUint32LE(gameState);
}

// Section 2: party. Count + selection index + per-character length-prefixed
// PoolradCharacter payload. Only the ACTIVE selection is persisted, as an INDEX
// (never a pointer). _targetCharacter / _nextCharacter are transient and are
// intentionally NOT round-tripped (OPEN, carried from 24.2); a stable map
// boundary never has one pending.
Common::Error saveParty(Common::Serializer &s, PoolradEngine &engine) {
	Common::List<::Goldbox::Data::PlayerCharacter *> &party = engine.getParty();

	uint16 count = static_cast<uint16>(party.size());
	s.syncAsUint16LE(count);

	uint16 selectionIndex = kNoSelectionIndex;
	{
		uint16 idx = 0;
		const ::Goldbox::Data::PlayerCharacter *sel = engine.getSelectedCharacter();
		for (auto it = party.begin(); it != party.end(); ++it, ++idx) {
			if (*it == sel) {
				selectionIndex = idx;
				break;
			}
		}
	}
	s.syncAsUint16LE(selectionIndex);

	for (auto it = party.begin(); it != party.end(); ++it) {
		Data::PoolradCharacter *pc =
			static_cast<Data::PoolradCharacter *>(*it);

		Common::MemoryWriteStreamDynamic charBuf(DisposeAfterUse::YES);
		pc->save(charBuf);

		uint32 len = static_cast<uint32>(charBuf.size());
		s.syncAsUint32LE(len);
		if (len)
			s.syncBytes(charBuf.getData(), len);
	}
	return Common::kNoError;
}

// Section 3: LocationContext. The location KIND has no VM memory field, so it
// cannot be rebuilt from the VM image alone and is serialized explicitly.
void saveLocation(Common::Serializer &s, PoolradEngine &engine) {
	LocationContext ctx = engine.getLocationContext();

	uint8 kind = static_cast<uint8>(ctx.kind);
	s.syncAsByte(kind);
	s.syncAsByte(ctx.region);
	s.syncAsByte(ctx.activeEclId);
	s.syncAsByte(ctx.activeGeoId);

	s.syncAsSByte(ctx.dungeonPos.x);
	s.syncAsSByte(ctx.dungeonPos.y);
	s.syncAsByte(ctx.dungeonDir);
	s.syncAsByte(ctx.wildernessPos.col);
	s.syncAsByte(ctx.wildernessPos.row);

	uint8 indoor = ctx.indoorRender ? 1 : 0;
	s.syncAsByte(indoor);
}

// Section 4: authoritative VM memory image (world/quest flags, clock,
// script/resource ids, dungeon/wilderness pos). Length is versioned so the
// load side can reject a mismatched size before applying.
Common::Error saveVmMemory(Common::Serializer &s, PoolradEngine &engine) {
	ECL::AddressSpace *mem = engine.getEclMemory();
	if (!mem) {
		warning("Goldbox native save: VM memory not available");
		return Common::Error(Common::kWritingFailed,
			"Goldbox: VM memory unavailable for native save");
	}

	uint32 length = ECL::AddressSpace::MEMORY_SIZE;
	s.syncAsUint32LE(length);

	Common::ScopedPtr<Common::MemoryReadStream> image(mem->openReadStream());
	if (!image) {
		return Common::Error(Common::kWritingFailed,
			"Goldbox: failed to read VM memory image");
	}
	byte *buf = new byte[length];
	image->read(buf, length);
	s.syncBytes(buf, length);
	delete[] buf;
	return Common::kNoError;
}

// Section 5: RNG seed. Common::RandomSource exposes only getSeed()/setSeed();
// getSeed() is the INITIAL seed, not the live MT internal state. We serialize
// the seed so a reload reproduces the same deterministic sequence from it.
// Exact live-state parity is an OPEN caveat (no public live-state accessor).
void saveRng(Common::Serializer &s, PoolradEngine &engine) {
	Common::RandomSource &rng = engine.getRandomSource();
	uint32 seed = rng.getSeed();
	s.syncAsUint32LE(seed);
}

Common::Error saveAll(Common::Serializer &s, PoolradEngine &engine) {
	s.matchBytes(kNativeSaveMagic, kNativeSaveMagicSize);
	s.syncVersion(kNativeSaveVersion);

	saveProfile(s, engine);

	Common::Error err = saveParty(s, engine);
	if (err.getCode() != Common::kNoError)
		return err;

	saveLocation(s, engine);

	err = saveVmMemory(s, engine);
	if (err.getCode() != Common::kNoError)
		return err;

	saveRng(s, engine);

	if (s.err()) {
		// Read-only / full / failed game-data write surfaces here truthfully
		// (Requirement 30.2 read-only = save-side): the stream's err() is set
		// when a write could not complete.
		return Common::Error(Common::kWritingFailed,
			"Goldbox: write failed during native save (disk full / read-only?)");
	}

	return Common::kNoError;
}

} // end anonymous namespace (save)

// ---------------------------------------------------------------------------
// LOAD side (24.3). Transactional: readInto a temporary -> validate -> commit.
// Nothing touches the live engine until commit().
// ---------------------------------------------------------------------------

Common::Error readInto(Common::SeekableReadStream &stream, LoadedState &out) {
	out.reset();

	// A short read on a MemoryReadStream does NOT set err(), only eos(). We
	// check eos() after reads so a truncated stream is caught truthfully.
	Common::Serializer s(&stream, nullptr);

	// Header: magic. A non-matching magic (including the old dummy payload) is
	// rejected before anything else is parsed.
	if (!s.matchBytes(kNativeSaveMagic, kNativeSaveMagicSize) || stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: not a native save (bad magic / legacy dummy payload)");
	}

	// Version. syncVersion returns false when the stored version is NEWER than
	// we support. There is only version 1 today.
	if (!s.syncVersion(kNativeSaveVersion) || stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: unsupported native save version");
	}

	// Section 1: profile.
	s.syncString(out.profileGameId);
	s.syncAsUint32LE(out.profilePlatform);
	s.syncAsUint32LE(out.profileFeatures);
	s.syncAsUint32LE(out.profileGameState);
	if (stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: native save truncated in profile section");
	}

	// Section 2: party (into NEW PoolradCharacter instances owned by out).
	uint16 count = 0;
	s.syncAsUint16LE(count);
	s.syncAsUint16LE(out.selectionIndex);
	if (stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: native save truncated in party header");
	}

	for (uint16 i = 0; i < count; ++i) {
		uint32 len = 0;
		s.syncAsUint32LE(len);
		if (stream.eos()) {
			return Common::Error(Common::kReadingFailed,
				"Goldbox: native save truncated before character payload");
		}

		Common::ScopedPtr<Data::PoolradCharacter> pc(new Data::PoolradCharacter());
		if (len) {
			byte *buf = new byte[len];
			s.syncBytes(buf, len);
			if (stream.eos()) {
				delete[] buf;
				return Common::Error(Common::kReadingFailed,
					"Goldbox: native save truncated in character payload");
			}
			Common::MemoryReadStream charStream(buf, len, DisposeAfterUse::NO);
			pc->load(charStream);
			delete[] buf;
		}
		// Ownership moves into out.party; released from the ScopedPtr so it is
		// not double-freed. out frees it if a later section errors.
		out.party.push_back(pc.release());
	}

	// Section 3: LocationContext.
	{
		uint8 kind = 0;
		s.syncAsByte(kind);
		s.syncAsByte(out.location.region);
		s.syncAsByte(out.location.activeEclId);
		s.syncAsByte(out.location.activeGeoId);
		s.syncAsSByte(out.location.dungeonPos.x);
		s.syncAsSByte(out.location.dungeonPos.y);
		s.syncAsByte(out.location.dungeonDir);
		s.syncAsByte(out.location.wildernessPos.col);
		s.syncAsByte(out.location.wildernessPos.row);
		uint8 indoor = 0;
		s.syncAsByte(indoor);
		out.location.kind = static_cast<LocationKind>(kind & 0x03);
		out.location.indoorRender = (indoor != 0);
		out.location.syncRegion();
		if (stream.eos()) {
			return Common::Error(Common::kReadingFailed,
				"Goldbox: native save truncated in location section");
		}
	}

	// Section 4: VM memory image into out's owned buffer (NOT live memory).
	s.syncAsUint32LE(out.vmImageLength);
	if (stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: native save truncated before VM image");
	}
	// Guard the allocation against a corrupt/huge length; validate() enforces
	// the exact size, but we must not allocate an arbitrary amount here.
	if (out.vmImageLength == 0 ||
	    out.vmImageLength > ECL::AddressSpace::MEMORY_SIZE) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: native save VM image length out of range");
	}
	out.vmImage = new byte[out.vmImageLength];
	s.syncBytes(out.vmImage, out.vmImageLength);
	if (stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: native save truncated in VM image");
	}

	// Section 5: RNG seed.
	s.syncAsUint32LE(out.rngSeed);
	if (stream.eos()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: native save truncated in RNG section");
	}

	return Common::kNoError;
}

Common::Error validate(const LoadedState &in, const PoolradEngine &engine) {
	// Profile (Requirement 30.2 wrong-profile). gameId must match exactly; a
	// save from another Goldbox game or a mismatched build is rejected.
	if (in.profileGameId != engine.getGameId()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save is for a different game (wrong profile)");
	}
	if (in.profilePlatform != static_cast<uint32>(engine.getPlatform())) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save is for a different platform (wrong profile)");
	}

	// Counts / selection (Requirement 30.2 bad counts). The party cannot exceed
	// the engine's MAX_CHARACTERS bound.
	if (in.party.size() > (uint)MAX_CHARACTERS) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save has too many party members");
	}
	if (in.selectionIndex != kNoSelectionIndex &&
	    in.selectionIndex >= in.party.size()) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save selection index out of range");
	}

	// VM image size (Requirement 30.2 size mismatch).
	if (in.vmImage == nullptr ||
	    in.vmImageLength != ECL::AddressSpace::MEMORY_SIZE) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save VM memory image size mismatch");
	}

	// Resource references (Requirement 30.2 missing-resource). The saved ECL
	// and GEO ids must resolve to real DAX blocks before we swap, otherwise the
	// derived rebuild (initializeMapRuntimeForState) would fault on a map that
	// no longer exists in the installed game data.
	//
	// OPEN (documented): this validates the top-level map resources only (the
	// active ECL script block and the active GEO block). A deeper walk of every
	// resource a map references (walldef/tile/pic chains) is not yet done; those
	// are rebuilt lazily by the runtime and a missing one would surface there
	// rather than here. The ids we can confirm from loadGameSlotX86's rebuild
	// path are the ECL and GEO block ids.
	PoolradEngine &mutableEngine = const_cast<PoolradEngine &>(engine);
	const uint8 eclId = in.location.activeEclId;
	const uint8 geoId = in.location.activeGeoId;

	Goldbox::Data::DaxBlock *eclBlock =
		mutableEngine.getDaxEcl().getBlockById(eclId);
	if (eclBlock == nullptr || eclBlock->_data.size() <= 2) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save references a missing map script (ECL resource)");
	}

	if (mutableEngine.getGeoBlockById(geoId) == nullptr) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: save references a missing map (GEO resource)");
	}

	return Common::kNoError;
}

void commit(LoadedState &in, PoolradEngine &engine) {
	// --- Swap the party. Free the old, move in the validated new ones. ---
	Common::List<::Goldbox::Data::PlayerCharacter *> &party = engine.getParty();
	engine.setSelectedCharacter(nullptr);
	engine.setTargetCharacter(nullptr);
	for (auto it = party.begin(); it != party.end(); ++it)
		delete *it;
	party.clear();

	for (uint i = 0; i < in.party.size(); ++i)
		party.push_back(in.party[i]);
	// Ownership transferred to the engine; clear so the LoadedState destructor
	// does not free (and does not double-free) the now-live characters.
	in.party.clear();

	if (in.selectionIndex != kNoSelectionIndex &&
	    in.selectionIndex < party.size()) {
		uint16 idx = 0;
		for (auto it = party.begin(); it != party.end(); ++it, ++idx) {
			if (idx == in.selectionIndex) {
				engine.setSelectedCharacter(*it);
				break;
			}
		}
	}

	// --- Apply the validated VM image into live memory. ---
	ECL::AddressSpace *mem = engine.getEclMemory();
	if (mem && in.vmImage && in.vmImageLength == ECL::AddressSpace::MEMORY_SIZE) {
		mem->loadBytes(0, Common::Span<const uint8>(in.vmImage, in.vmImageLength));
	}

	// --- Apply location + RNG. ---
	engine.setLocationContext(in.location);
	engine.getRandomSource().setSeed(in.rngSeed);

	// --- Trigger the derived rebuild via a state transition, mirroring
	// loadGameSlotX86's final setGameState: onGameStateEnter ->
	// initializeMapRuntimeForState reloads the ECL/GEO blocks, runs ECL ONINIT,
	// refreshes the viewport and rebuilds caches. A transient saved state
	// resolves to the map state its location implies. ---
	GameState resolvedState =
		static_cast<GameState>(in.profileGameState);
	if (resolvedState != GS_DUNGEON_MAP && resolvedState != GS_WILDERNESS_MAP) {
		resolvedState = PoolradEngine::mapStateForLocation(in.location);
	}
	engine.setGameState(resolvedState);
}

Common::Error sync(Common::Serializer &s, PoolradEngine &engine) {
	// SAVE path only. The LOAD path is transactional and runs through the raw
	// stream (readInto/validate/commit), routed from
	// PoolradEngine::loadGameStream so end-of-stream truncation is detectable.
	if (s.isSaving())
		return saveAll(s, engine);

	// A Serializer-only load reaching here would hide the stream (no eos check)
	// and could not be transactional. The engine routes loads to loadNative()
	// instead, so this is an internal misuse guard.
	warning("Goldbox native save: load must route through the transactional path");
	return Common::Error(Common::kReadingFailed,
		"Goldbox: native load requires the transactional stream path");
}

Common::Error loadNative(Common::SeekableReadStream &stream, PoolradEngine &engine) {
	// Transactional load: parse into a self-contained temporary, validate it,
	// and only then swap it into the live engine. A failure at readInto or
	// validate returns the first error with the previous game fully intact.
	LoadedState staged;

	Common::Error err = readInto(stream, staged);
	if (err.getCode() != Common::kNoError)
		return err; // previous game untouched; staged frees what it parsed

	err = validate(staged, engine);
	if (err.getCode() != Common::kNoError)
		return err; // previous game untouched

	commit(staged, engine); // swap in; staged's chars now owned by the engine
	return Common::kNoError;
}

} // namespace NativeSave
} // namespace Poolrad
} // namespace Goldbox

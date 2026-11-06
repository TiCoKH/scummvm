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

#include "common/memstream.h"
#include "common/ptr.h"
#include "common/span.h"
#include "common/textconsole.h"

#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/data/poolrad_character.h"

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

namespace {

// Section 1: header/profile. Captures the game/release/platform identity and
// the active game state so 24.3 can reject a wrong-profile load. On load the
// magic and version are validated by the caller before this is reached.
void syncProfile(Common::Serializer &s, PoolradEngine &engine) {
	Common::String gameId = engine.getGameId();
	s.syncString(gameId);

	uint32 platform = static_cast<uint32>(engine.getPlatform());
	s.syncAsUint32LE(platform);

	uint32 features = engine.getFeatures();
	s.syncAsUint32LE(features);

	uint32 gameState = static_cast<uint32>(engine.getGameState());
	s.syncAsUint32LE(gameState);
	// Note: profile fields are read back but NOT yet enforced here; 24.3
	// compares them against the running engine and rejects a mismatch.
}

// Section 2: party. Count + per-character payload + selection index.
//
// Each character is serialized through PoolradCharacter::save/load (the legacy
// per-character field I/O: stats/items/effects/spells) into a length-prefixed
// byte block, so the native schema reuses the existing serialization instead
// of re-encoding fields here. Only the ACTIVE selection is persisted, as an
// INDEX into the party order (never a pointer). _targetCharacter and
// _nextCharacter are transient command/combat targets and are intentionally
// NOT round-tripped (OPEN for 24.3 if a use case needs them); a stable map
// boundary never has one pending.
Common::Error syncParty(Common::Serializer &s, PoolradEngine &engine) {
	Common::List<::Goldbox::Data::PlayerCharacter *> &party = engine.getParty();

	if (s.isSaving()) {
		uint16 count = static_cast<uint16>(party.size());
		s.syncAsUint16LE(count);

		// Selection as an index into the party order (kNoSelectionIndex = none).
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

	// Loading.
	uint16 count = 0;
	s.syncAsUint16LE(count);

	uint16 selectionIndex = kNoSelectionIndex;
	s.syncAsUint16LE(selectionIndex);

	// Rebuild the party from the stream. Derived caches/views are rebuilt by
	// 24.3; here we restore the authoritative character objects and order.
	for (auto it = party.begin(); it != party.end(); ++it)
		delete *it;
	party.clear();
	engine.setSelectedCharacter(nullptr);
	engine.setTargetCharacter(nullptr);

	for (uint16 i = 0; i < count; ++i) {
		uint32 len = 0;
		s.syncAsUint32LE(len);

		Data::PoolradCharacter *pc = new Data::PoolradCharacter();
		if (len) {
			byte *buf = new byte[len];
			s.syncBytes(buf, len);
			Common::MemoryReadStream charStream(buf, len, DisposeAfterUse::NO);
			pc->load(charStream);
			delete[] buf;
		}
		party.push_back(pc);
	}

	// Restore the selection by index. An out-of-range index is handled safely
	// as "no selection" here; full validation/clamping policy is 24.3.
	if (selectionIndex != kNoSelectionIndex && selectionIndex < party.size()) {
		uint16 idx = 0;
		for (auto it = party.begin(); it != party.end(); ++it, ++idx) {
			if (idx == selectionIndex) {
				engine.setSelectedCharacter(*it);
				break;
			}
		}
	}

	return Common::kNoError;
}

// Section 3: LocationContext. Serialized explicitly because the location KIND
// has no VM memory field and therefore cannot be rebuilt from the VM image
// alone. All fields are plain data.
void syncLocation(Common::Serializer &s, PoolradEngine &engine) {
	LocationContext ctx = engine.getLocationContext();

	uint8 kind = static_cast<uint8>(ctx.kind);
	s.syncAsByte(kind);
	s.syncAsByte(ctx.region);
	s.syncAsByte(ctx.activeEclId);
	s.syncAsByte(ctx.activeGeoId);

	// MapPos is signed int8 x/y; TilePos is uint8 col/row.
	s.syncAsSByte(ctx.dungeonPos.x);
	s.syncAsSByte(ctx.dungeonPos.y);
	s.syncAsByte(ctx.dungeonDir);
	s.syncAsByte(ctx.wildernessPos.col);
	s.syncAsByte(ctx.wildernessPos.row);

	uint8 indoor = ctx.indoorRender ? 1 : 0;
	s.syncAsByte(indoor);

	if (s.isLoading()) {
		ctx.kind = static_cast<LocationKind>(kind & 0x03);
		ctx.indoorRender = (indoor != 0);
		ctx.syncRegion();
		engine.setLocationContext(ctx);
	}
}

// Section 4: authoritative VM memory image. The 64K ECL address space holds
// world/quest flags, the clock (0x49C5..0x49CC), script/resource ids, dungeon/
// wilderness position and search/preparation bytes. The length is versioned so
// 24.3 can reject a mismatched size. On save we stream the whole buffer out;
// on load we read it into a heap buffer and write it back via loadBytes().
// 24.3 will validate the captured image BEFORE applying it; the capture here
// is kept clean (a single contiguous buffer) for that purpose.
Common::Error syncVmMemory(Common::Serializer &s, PoolradEngine &engine) {
	ECL::AddressSpace *mem = engine.getEclMemory();
	if (!mem) {
		// No runtime VM yet: native save/load is only offered at a stable map
		// boundary where the VM exists, so this is an honest failure.
		warning("Goldbox native save: VM memory not available (GB13-033 24.2)");
		return Common::Error(Common::kWritingFailed,
			"Goldbox: VM memory unavailable for native save");
	}

	uint32 length = ECL::AddressSpace::MEMORY_SIZE;
	s.syncAsUint32LE(length);

	if (s.isSaving()) {
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

	// Loading.
	if (length != ECL::AddressSpace::MEMORY_SIZE) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: VM memory image size mismatch");
	}
	byte *buf = new byte[length];
	s.syncBytes(buf, length);
	// 24.2 writes the captured image straight back. 24.3 inserts validation
	// (profile/flag sanity, bounds) against this clean buffer BEFORE applying.
	mem->loadBytes(0, Common::Span<const uint8>(buf, length));
	delete[] buf;
	return Common::kNoError;
}

// Section 5: RNG seed. Common::RandomSource exposes only getSeed()/setSeed();
// getSeed() returns the INITIAL seed, not the live MT internal state. We
// serialize the seed so a reload reproduces the same deterministic sequence
// from that seed. Exact live-state parity is an OPEN caveat for 24.3 (there is
// no public live-state accessor on RandomSource to serialize).
void syncRng(Common::Serializer &s, PoolradEngine &engine) {
	Common::RandomSource &rng = engine.getRandomSource();
	uint32 seed = rng.getSeed();
	s.syncAsUint32LE(seed);
	if (s.isLoading())
		rng.setSeed(seed);
}

} // end anonymous namespace

Common::Error sync(Common::Serializer &s, PoolradEngine &engine) {
	// Header: magic + version. On load, a non-matching magic (including the old
	// dummy payload) or an unsupported version is rejected clearly.
	if (!s.matchBytes(kNativeSaveMagic, kNativeSaveMagicSize)) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: not a native save (bad magic / legacy dummy payload)");
	}

	// syncVersion returns false when the stored version is NEWER than what we
	// support. An older-but-known version would still load (fields are gated by
	// minVersion); there is only version 1 today.
	if (!s.syncVersion(kNativeSaveVersion)) {
		return Common::Error(Common::kReadingFailed,
			"Goldbox: unsupported native save version");
	}

	syncProfile(s, engine);

	Common::Error err = syncParty(s, engine);
	if (err.getCode() != Common::kNoError)
		return err;

	syncLocation(s, engine);

	err = syncVmMemory(s, engine);
	if (err.getCode() != Common::kNoError)
		return err;

	syncRng(s, engine);

	if (s.err()) {
		return Common::Error(s.isLoading() ? Common::kReadingFailed
		                                   : Common::kWritingFailed,
			"Goldbox: stream error during native save/load");
	}

	return Common::kNoError;
}

} // namespace NativeSave
} // namespace Poolrad
} // namespace Goldbox

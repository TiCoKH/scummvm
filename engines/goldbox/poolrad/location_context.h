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
 */

#ifndef GOLDBOX_POOLRAD_LOCATION_CONTEXT_H
#define GOLDBOX_POOLRAD_LOCATION_CONTEXT_H

#include "common/scummsys.h"
#include "goldbox/core/coords.h"
#include "goldbox/ecl/pending_operation.h"

namespace Goldbox {

struct RuntimeMapSnapshot;

namespace Poolrad {

/**
 * GB13-021 — authoritative location TYPE owner.
 *
 * The original conflated the location TYPE (dungeon vs. the three wilderness
 * regions) with the indoor render flag: both were read from the single indoor
 * byte (kVmFieldIndoorModeFlag), so wilderness types 2/3/4 were unreachable
 * and an outdoor-looking dungeon scene could not be distinguished.
 *
 * LocationContext separates the two. It is the engine-owned authoritative
 * source of the location kind/region; RuntimeMapSnapshot.mapType is DERIVED
 * from it and read-only. The indoor-render flag stays independent and still
 * mirrors kVmFieldIndoorModeFlag (the VM owns the render mode; the engine owns
 * the location kind, since no distinct VM field holds the TYPE).
 *
 * Field ownership:
 *   kind          — engine-owned (no distinct VM field exists for the TYPE).
 *   region        — derived from kind (0/13/26 atlas offset: west/central/east).
 *   activeEclId   — mirrors the saved ECL id VM field (kVmFieldSavedEclId).
 *   activeGeoId   — mirrors the GEO block id VM field (kVmFieldGeoBlockId).
 *   dungeonPos    — mirrors the dungeon X/Y VM globals.
 *   dungeonDir    — mirrors the dungeon dir VM global (0..3).
 *   wildernessPos — mirrors the wilderness X/Y VM fields.
 *   indoorRender  — mirrors kVmFieldIndoorModeFlag; INDEPENDENT of kind.
 */
enum class LocationKind : uint8 {
	Dungeon = 0,
	WildernessWest,
	WildernessCentral,
	WildernessEast
};

/**
 * ECL script ids for the three wilderness regions (= the wilderness map ids
 * 0x19/0x1A/0x1B). NewEclDone assigns a wilderness kind only when the active
 * script id is one of these AND the 3D view flag is zero (indoor flag clear).
 */
enum WildernessScriptId {
	kScriptWildernessWest    = 25, // 0x19
	kScriptWildernessCentral = 26, // 0x1A
	kScriptWildernessEast    = 27  // 0x1B
};

struct LocationContext {
	LocationKind kind;
	uint8   region;        // 0/13/26 atlas offset derived from kind
	uint8   activeEclId;
	uint8   activeGeoId;
	MapPos  dungeonPos;
	uint8   dungeonDir;
	TilePos wildernessPos;
	bool    indoorRender;  // independent of kind

	LocationContext()
		: kind(LocationKind::Dungeon), region(0), activeEclId(0),
		  activeGeoId(0), dungeonPos(), dungeonDir(0), wildernessPos(),
		  indoorRender(true) {}

	explicit LocationContext(LocationKind k)
		: kind(k), region(regionForKind(k)), activeEclId(0), activeGeoId(0),
		  dungeonPos(), dungeonDir(0), wildernessPos(),
		  indoorRender(k == LocationKind::Dungeon) {}

	bool isWilderness() const { return kind != LocationKind::Dungeon; }

	/** Keep region consistent with kind (single boundary for 0/13/26). */
	void syncRegion() { region = regionForKind(kind); }

	/** Atlas offset for a kind: Dungeon/West=0, Central=13, East=26. */
	static uint8 regionForKind(LocationKind k) {
		switch (k) {
		case LocationKind::WildernessCentral:
			return 13;
		case LocationKind::WildernessEast:
			return 26;
		default:
			return 0;
		}
	}

	/**
	 * Classify the location TYPE from the active script id and the 3D-view
	 * flag, per NewEclDone (GB13-021 AC 2). The caller starts every on-load
	 * entry as Dungeon; only when @p view3dFlag is zero AND @p activeScriptId
	 * is one of the three wilderness scripts does it become a wilderness kind.
	 *
	 * @p view3dFlag is the indoor-render / 3D-view byte: zero means the 3D
	 * first-person view is NOT active (outdoor), matching indoorFlag == 0.
	 */
	static LocationKind classify(uint8 activeScriptId, uint8 view3dFlag) {
		if (view3dFlag != 0)
			return LocationKind::Dungeon;
		switch (activeScriptId) {
		case kScriptWildernessWest:
			return LocationKind::WildernessWest;
		case kScriptWildernessCentral:
			return LocationKind::WildernessCentral;
		case kScriptWildernessEast:
			return LocationKind::WildernessEast;
		default:
			return LocationKind::Dungeon;
		}
	}

	/**
	 * Legacy numeric map type at one boundary (no raw numeric tests scattered
	 * in views). Dungeon = 1; wilderness west/central/east = 2/3/4. This is the
	 * value RuntimeMapSnapshot.mapType is populated with.
	 */
	uint8 toLegacyMapType() const {
		switch (kind) {
		case LocationKind::WildernessWest:
			return 2;
		case LocationKind::WildernessCentral:
			return 3;
		case LocationKind::WildernessEast:
			return 4;
		default:
			return 1;
		}
	}

	/** Inverse of toLegacyMapType() for legacy save/load reconstruction. */
	static LocationKind kindFromLegacyMapType(uint8 mapType) {
		switch (mapType) {
		case 2:
			return LocationKind::WildernessWest;
		case 3:
			return LocationKind::WildernessCentral;
		case 4:
			return LocationKind::WildernessEast;
		default:
			return LocationKind::Dungeon;
		}
	}

	/**
	 * Convert to the ecl-layer ReturnContextRef so a service PendingOperation
	 * can remember the caller location without ecl/ depending on poolrad/
	 * (layering option (b)). The scalars carry kind, region and the active ECL
	 * id; coordinates/render flag are restored from the live/persisted VM
	 * state, so the round-trip preserves the fields the return path needs.
	 */
	ECL::ReturnContextRef toReturnContextRef() const {
		ECL::ReturnContextRef ref;
		ref.kind = static_cast<uint8>(kind);
		ref.region = region;
		ref.activeId = activeEclId;
		return ref;
	}

	/** Rebuild the kind/region/active-id scalars from a ReturnContextRef. */
	static LocationContext fromReturnContextRef(const ECL::ReturnContextRef &ref) {
		LocationContext ctx;
		ctx.kind = static_cast<LocationKind>(ref.kind & 0x03);
		ctx.activeEclId = static_cast<uint8>(ref.activeId & 0xFF);
		ctx.syncRegion();
		ctx.indoorRender = (ctx.kind == LocationKind::Dungeon);
		return ctx;
	}
};

/**
 * Derive a LocationContext from a RuntimeMapSnapshot (read-only).
 *
 * The snapshot is the derived, read-only view captured by CaptureMapSnapshot;
 * this rebuilds the authoritative context fields from it without touching any
 * VM/engine state. Declared here (defined in location_context.cpp) so views
 * and save/load have one boundary for the mapType derivation.
 */
LocationContext deriveLocationContextFromSnapshot(
		const ::Goldbox::RuntimeMapSnapshot &snapshot);

/** Legacy numeric mapType derived from a context (1=dungeon, 2/3/4=wild). */
uint8 deriveMapTypeFromContext(const LocationContext &ctx);

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_LOCATION_CONTEXT_H

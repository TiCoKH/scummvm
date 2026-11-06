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
 */

#ifndef GOLDBOX_POOLRAD_WILDERNESS_ATLAS_H
#define GOLDBOX_POOLRAD_WILDERNESS_ATLAS_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {

/**
 * GB13-024 shared wilderness-atlas grid seam (Requirement 39).
 *
 * The DOS overworld terrain map is a 44x36 grid of one-based atlas tile ids
 * (the per-cell MAP_DrawAreaMapBlock terrain grid). The GRAPHICS for the 34
 * terrain tiles ship in WILDCOM.DAX, but the per-cell LAYOUT grid is NOT in
 * the codebase: BattlefieldMap::kWildernessTilemap is an empty {{0}}
 * placeholder and WILDCOM ships only the tile art.
 *
 * ----------------------------------------------------------------------------
 * OPEN (trace-gated DOS-reference DATA - deliberately NOT fabricated):
 *   the 44x36 one-based terrain-id grid itself. AI-GUIDELINES forbids inventing
 *   this. terrainIdAt() returns 0 (baseline/empty cell) for every cell until
 *   the confirmed grid is dropped into kGrid below.
 * ----------------------------------------------------------------------------
 *
 * This is the SINGLE grid-read seam both the wilderness DRAWING module
 * (WildernessMap, GB13-024) and the wilderness TRAVEL rules
 * (HostWildernessEnvironment::readAtlasTerrain, GB13-023) read through, so once
 * the grid data lands both share one source. It lives in poolrad/ with no
 * dependency on gfx/ or combat/ to avoid a layering cycle.
 *
 * Coordinate convention mirrors WildernessTravel::atlasCell: atlasX in
 * 0..kCols-1 already includes the region offset (0/13/26); atlasY in
 * 0..kRows-1. Out-of-range cells return 0 so no lookup ever indexes out of
 * range. The returned id is ONE-BASED (the drawn tile slot is id - 1; see
 * WildernessMap::idToTileSlot).
 */
class WildernessAtlas {
public:
	/** 44x36 overworld atlas dimensions (mirror the combat constants). */
	static const int kCols = 44;
	static const int kRows = 36;

	/**
	 * One-based terrain id at a bounds-validated atlas cell, or 0 when the
	 * cell is out of range OR when the grid data has not been populated yet
	 * (the current state: the whole grid is empty, so this returns 0 for
	 * every cell - a clean baseline/empty fallback).
	 */
	static uint8 terrainIdAt(int atlasX, int atlasY);

	/**
	 * True once the 44x36 grid DATA has been populated (currently false: the
	 * grid is the empty OPEN placeholder). Callers may use this to diagnose
	 * the baseline/empty fallback path; drawing falls back cleanly regardless.
	 */
	static bool isGridPopulated();
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_WILDERNESS_ATLAS_H

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

#include "goldbox/poolrad/wilderness_travel.h"

namespace Goldbox {
namespace Poolrad {

// --- DOS-reference data (Pool of Radiance DOS v1.3) -----------------------
//
// Blocked overworld terrain ids. These are the fixed impassable classes of the
// 44x36 wilderness atlas (mountains / ruins walls / sea / etc.), taken from the
// GB13-023 design terrain table. They are encoded as a static const array (no
// runtime global constructor) rather than folded into inline magic tests.
// Ranges per the design: 78; 126..128; 130..132; 168..172; 174..179; 188;
// 221..222; 225..233; 245.
static const uint8 kBlockedTerrainIds[] = {
	78,
	126, 127, 128,
	130, 131, 132,
	168, 169, 170, 171, 172,
	174, 175, 176, 177, 178, 179,
	188,
	221, 222,
	225, 226, 227, 228, 229, 230, 231, 232, 233,
	245
};
static const int kBlockedTerrainCount =
	(int)(sizeof(kBlockedTerrainIds) / sizeof(kBlockedTerrainIds[0]));

// River overworld terrain ids. Passable ONLY when the Yarash "pyramid left"
// flag gates them open; otherwise blocked. DOS-reference data: 102..108; 114.
static const uint8 kRiverTerrainIds[] = {
	102, 103, 104, 105, 106, 107, 108,
	114
};
static const int kRiverTerrainCount =
	(int)(sizeof(kRiverTerrainIds) / sizeof(kRiverTerrainIds[0]));

WildernessTravel::Compass WildernessTravel::arrowToCompass(Arrow arrow) {
	// The ONE arrow->compass conversion boundary. Directional travel, NOT a
	// dungeon facing rotation.
	switch (arrow) {
	case ARROW_DOWN:
		return TRAVEL_SOUTH;
	case ARROW_LEFT:
		return TRAVEL_WEST;
	case ARROW_RIGHT:
		return TRAVEL_EAST;
	case ARROW_UP:
	default:
		return TRAVEL_NORTH;
	}
}

void WildernessTravel::compassStep(Compass dir, int &dx, int &dy) {
	// Reuse the engine 8-way delta table (y: north = -1) at one boundary.
	Direction wire = DIR_N;
	switch (dir) {
	case TRAVEL_SOUTH:
		wire = DIR_S;
		break;
	case TRAVEL_WEST:
		wire = DIR_W;
		break;
	case TRAVEL_EAST:
		wire = DIR_E;
		break;
	case TRAVEL_NORTH:
	default:
		wire = DIR_N;
		break;
	}
	dx = kDirDeltaX[wire];
	dy = kDirDeltaY[wire];
}

bool WildernessTravel::isBlockedTerrain(uint8 terrainId) {
	for (int i = 0; i < kBlockedTerrainCount; ++i) {
		if (kBlockedTerrainIds[i] == terrainId)
			return true;
	}
	return false;
}

bool WildernessTravel::isRiverTerrain(uint8 terrainId) {
	for (int i = 0; i < kRiverTerrainCount; ++i) {
		if (kRiverTerrainIds[i] == terrainId)
			return true;
	}
	return false;
}

bool WildernessTravel::isRiverGatedOpen(uint8 terrainId, bool yarashPyramidLeft) {
	return isRiverTerrain(terrainId) && yarashPyramidLeft;
}

bool WildernessTravel::isPassable(uint8 terrainId, bool yarashPyramidLeft) {
	if (isBlockedTerrain(terrainId))
		return false;
	if (isRiverTerrain(terrainId))
		return yarashPyramidLeft;
	return true;
}

bool WildernessTravel::clampLocal(int &localX, int &localY) {
	bool clamped = false;
	if (localX > kLocalXMax) { localX = kLocalXMax; clamped = true; }
	if (localX < kLocalXMin) { localX = kLocalXMin; clamped = true; }
	if (localY > kLocalYMax) { localY = kLocalYMax; clamped = true; }
	if (localY < kLocalYMin) { localY = kLocalYMin; clamped = true; }
	return clamped;
}

bool WildernessTravel::atlasCell(int clampedLocalX, int clampedLocalY,
		uint8 region, int &atlasX, int &atlasY) {
	// EXPLICIT SIGNED math then a bounds check BEFORE any lookup: the atlas is
	// never indexed out of range. region is 0/13/26; a clamped local x (0..15)
	// plus the east offset (26) reaches at most 41 < 44.
	const int ax = clampedLocalX + (int)region;
	const int ay = clampedLocalY;
	if (ax < 0 || ax >= kAtlasCols || ay < 0 || ay >= kAtlasRows)
		return false;
	atlasX = ax;
	atlasY = ay;
	return true;
}

WildernessTravel::MoveResult WildernessTravel::step(Environment &env,
		Compass dir) {
	MoveResult result;

	int localX = 0;
	int localY = 0;
	env.currentLocal(localX, localY);

	// Prospective destination with EXPLICIT SIGNED arithmetic (may go
	// negative) before clamping to the independent-coordinate ranges.
	int dx = 0;
	int dy = 0;
	compassStep(dir, dx, dy);
	int destX = localX + dx;
	int destY = localY + dy;

	result.clamped = clampLocal(destX, destY);
	result.destX = destX;
	result.destY = destY;

	const uint8 region = env.regionOffset();

	// Bounds-validate the atlas cell BEFORE reading. A rejected cell is treated
	// as blocked (no move, no time) so lookup never runs out of range.
	int atlasX = 0;
	int atlasY = 0;
	if (!atlasCell(destX, destY, region, atlasX, atlasY)) {
		result.permitted = false;
		result.moved = false;
		return result;
	}

	const uint8 terrainId = env.readAtlasTerrain(atlasX, atlasY);
	result.terrainId = terrainId;

	const bool permitted = isPassable(terrainId, env.yarashPyramidLeft());
	result.permitted = permitted;

	if (!permitted) {
		// Blocked terrain or a closed river: do NOT move and do NOT advance
		// time.
		result.moved = false;
		return result;
	}

	// Permitted: commit the clamped destination, advance 12 HOURS EVEN IF the
	// clamp left the coordinates unchanged, then run the script transitions in
	// the original order.
	result.moved = (destX != localX || destY != localY);
	env.commitLocal(destX, destY);
	env.advanceTwelveHours();
	env.runScriptTransitions();
	return result;
}

} // namespace Poolrad
} // namespace Goldbox

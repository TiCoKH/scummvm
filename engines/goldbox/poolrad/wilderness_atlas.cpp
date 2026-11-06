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

#include "goldbox/poolrad/wilderness_atlas.h"

namespace Goldbox {
namespace Poolrad {

// OPEN (trace-gated DOS-reference DATA): the 44x36 one-based terrain-id grid.
// It is NOT in the codebase (BattlefieldMap::kWildernessTilemap is an empty
// {{0}} placeholder and WILDCOM.DAX ships only the tile GRAPHICS). Per
// AI-GUIDELINES we do NOT fabricate it. Encoded here as a static const grid
// (no runtime global constructor) so the real confirmed data drops straight
// in; until then every cell is 0 => a baseline/empty draw and a passable
// travel default. When the data lands, flip kGridPopulated to true.
static const uint8 kGrid[WildernessAtlas::kRows][WildernessAtlas::kCols] = {
	{ 0 }
};

static const bool kGridPopulated = false;

uint8 WildernessAtlas::terrainIdAt(int atlasX, int atlasY) {
	if (atlasX < 0 || atlasX >= kCols || atlasY < 0 || atlasY >= kRows)
		return 0;
	return kGrid[atlasY][atlasX];
}

bool WildernessAtlas::isGridPopulated() {
	return kGridPopulated;
}

} // namespace Poolrad
} // namespace Goldbox

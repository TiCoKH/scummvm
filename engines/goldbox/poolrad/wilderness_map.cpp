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

#include "goldbox/poolrad/wilderness_map.h"
#include "goldbox/poolrad/wilderness_atlas.h"

namespace Goldbox {
namespace Poolrad {

// The read-only nine-case site decision table (Req 39.2). No runtime global
// constructor: a static const array of POD SiteCase.
//
// OPEN (trace-gated DOS-reference DATA): the triggerId and overrideId of each
// case, and which cases carry a replacement branch, are NOT confirmed and are
// deliberately NOT fabricated. The table SHAPE is the confirmable structure:
// nine indexed cases (indices 0..8), each checked before and after its exact
// quest condition; cases 1 and 2 (indices 1 and 2) have NO explicit
// replacement branch so they NEVER substitute (hasReplacement = false). Until
// the ids/conditions are traced every entry ships inert (triggerId 0,
// hasReplacement false), so the resolver changes nothing and all cells stay
// baseline. We do NOT invent extra sites for cases 1 and 2.
static const WildernessMap::SiteCase kSiteCases[WildernessMap::kSiteCount] = {
	// siteIndex, triggerId, overrideId, hasReplacement
	{ 0, 0, 0, false }, // OPEN: id/condition
	{ 1, 0, 0, false }, // case 1: no replacement branch (confirmed shape)
	{ 2, 0, 0, false }, // case 2: no replacement branch (confirmed shape)
	{ 3, 0, 0, false }, // OPEN: id/condition
	{ 4, 0, 0, false }, // OPEN: id/condition
	{ 5, 0, 0, false }, // OPEN: id/condition
	{ 6, 0, 0, false }, // OPEN: id/condition
	{ 7, 0, 0, false }, // OPEN: id/condition
	{ 8, 0, 0, false }  // OPEN: id/condition
};

// --- Grid-read seam -------------------------------------------------------

uint8 WildernessMap::AtlasGridReader::idAt(int atlasX, int atlasY) {
	return WildernessAtlas::terrainIdAt(atlasX, atlasY);
}

// --- Draw geometry (Req 39.1) ---------------------------------------------

bool WildernessMap::clampViewportOrigin(int &originX, int &originY) {
	bool clamped = false;
	if (originX < kMapWindowXMin) { originX = kMapWindowXMin; clamped = true; }
	if (originX > kMapWindowXMax) { originX = kMapWindowXMax; clamped = true; }
	if (originY < kMapWindowYMin) { originY = kMapWindowYMin; clamped = true; }
	if (originY > kMapWindowYMax) { originY = kMapWindowYMax; clamped = true; }
	return clamped;
}

int WildernessMap::idToTileSlot(uint8 oneBasedId) {
	// SUBTRACT ONE exactly ONCE here. id 0 == baseline/empty (NOT slot -1 used
	// as an index); id N>=1 == slot N-1. The blitter must NOT subtract again.
	if (oneBasedId == 0)
		return kNoTileSlot;
	return (int)oneBasedId - 1;
}

// --- Site-override resolver (Req 39.2) -------------------------------------

const WildernessMap::SiteCase *WildernessMap::siteTable() {
	return kSiteCases;
}

uint8 WildernessMap::resolveSiteOverride(uint8 baseId, QuestPredicate &quest) {
	// Read-only: find the indexed case whose trigger id matches the baseline
	// cell. An atlas id with no case stays baseline. A case with no replacement
	// branch (cases 1 and 2) never substitutes. A case WITH a replacement
	// substitutes its override ONLY when its quest condition holds (checked
	// before and after, as the original does). Mutates nothing.
	if (baseId == 0)
		return 0;

	for (int i = 0; i < kSiteCount; ++i) {
		const SiteCase &c = kSiteCases[i];
		if (c.triggerId == 0 || c.triggerId != baseId)
			continue;
		if (!c.hasReplacement)
			return baseId; // cases 1/2 shape: baseline, no substitution
		if (quest.questConditionMet(c.siteIndex))
			return c.overrideId;
		return baseId; // condition not met: baseline tile
	}
	return baseId; // no case for this id: baseline tile
}

// --- Window builder (Req 39.1 + 39.2) --------------------------------------

WildernessMap::Window WildernessMap::buildWindow(int originAtlasX,
		int originAtlasY, GridReader &grid, QuestPredicate &quest) {
	Window window;

	// Clamp the window origin to x 0..39 / y 0..31 FIRST.
	int ox = originAtlasX;
	int oy = originAtlasY;
	clampViewportOrigin(ox, oy);

	for (int row = 0; row < kWindowCells; ++row) {
		for (int col = 0; col < kWindowCells; ++col) {
			WindowCell &cell = window.cells[row][col];
			const int ax = ox + col;
			const int ay = oy + row;
			cell.atlasX = ax;
			cell.atlasY = ay;
			cell.inRange = (ax >= 0 && ax < kAtlasCols &&
				ay >= 0 && ay < kAtlasRows);

			const uint8 baseId = cell.inRange ? grid.idAt(ax, ay) : kEmptyId;
			cell.baseId = baseId;
			cell.finalId = resolveSiteOverride(baseId, quest);
			cell.tileSlot = idToTileSlot(cell.finalId);
		}
	}
	return window;
}

// --- Marker (Req 39.4) -----------------------------------------------------

WildernessMap::MarkerCell WildernessMap::markerCell(int prospectiveOriginX,
		int prospectiveOriginY, int partyAtlasX, int partyAtlasY) {
	MarkerCell marker;

	// Clamp the ORIGIN only; the party cell is NOT clamped. The marker is the
	// party cell relative to the clamped origin, computed INDEPENDENTLY of the
	// origin clamp so it drifts toward the window edge at map borders.
	int ox = prospectiveOriginX;
	int oy = prospectiveOriginY;
	clampViewportOrigin(ox, oy);

	marker.col = partyAtlasX - ox;
	marker.row = partyAtlasY - oy;
	marker.visible = (marker.col >= 0 && marker.col < kWindowCells &&
		marker.row >= 0 && marker.row < kWindowCells);
	return marker;
}

// --- YarashComplete one-time world-state op (Req 39.4) ---------------------

bool WildernessMap::shouldRunYarashComplete(WorldState &world) {
	// Gate: mission == 254 AND the pyramid-left flag is still 0 (closed).
	return world.mission() == kMissionYarashComplete &&
		!world.yarashPyramidLeft();
}

bool WildernessMap::runYarashComplete(WorldState &world) {
	if (!shouldRunYarashComplete(world))
		return false; // wrong mission or already open: idempotent no-op

	// One-time world change: OPEN the river by flipping the persistent
	// pyramid-left flag. Guarded by that same persistent flag, so a redraw /
	// save-load re-entry hits the shouldRun guard above and does nothing.
	world.setYarashPyramidLeft(true);
	return true;
}

} // namespace Poolrad
} // namespace Goldbox

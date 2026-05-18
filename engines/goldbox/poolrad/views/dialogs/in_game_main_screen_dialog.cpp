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

#include "goldbox/gfx/first_person_renderer.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/views/dialogs/in_game_main_screen_dialog.h"
#include "goldbox/runtime/runtime_exchange.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

// Wilderness map-type X offsets used by MAP_3DColorUpdate / MAP_DrawAreaMapBlock.
// Verified from wilderness runtime paths in legacy code:
//   type 2 -> +0, type 3 -> +13, type 4 -> +26.
static const int kWildernessMapXOffsets[8] = {
	0, 0, 0, 13, 26, 0, 0, 0
};

static int clampInt(int v, int lo, int hi) {
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

InGameMainScreenDialog::InGameMainScreenDialog(const Common::String &name)
	: Dialog(name) {
	setBounds(Window(0, 0, 39, 24));
}

void InGameMainScreenDialog::setDrawMode(DrawMode mode) {
	if (_mode == mode)
		return;

	_mode = mode;
	redraw();
}

uint8 InGameMainScreenDialog::effectiveMapTypeFromRuntime() const {
	if (!::Goldbox::Poolrad::g_engine)
		return (_mode == kModeWilderness) ? 2 : 1;

	const RuntimeExchange *exchange =
		::Goldbox::Poolrad::g_engine->getRuntimeExchange();
	if (!exchange)
		return (_mode == kModeWilderness) ? 2 : 1;

	::Goldbox::RuntimeMapSnapshot snapshot;
	if (!exchange->captureMapSnapshot(snapshot) || !snapshot.valid)
		return (_mode == kModeWilderness) ? 2 : 1;

	uint8 mapType = snapshot.mapType;
	if (snapshot.indoorMode)
		mapType = 1;

	return mapType;
}

void InGameMainScreenDialog::drawWildernessAreaBlock(Surface &s, uint8 anchorX,
		uint8 anchorY) {
	// Original MAP_DrawAreaMapBlock draws 5x5 cells as 3x3 tiles in the left
	// panel. Here we keep that geometry and paint a deterministic fallback until
	// terrain-tile lookup tables are promoted into typed runtime data.
	const int originX = 3 * 8 + 1;
	const int originY = 3 * 8 + 1;
	const int cellSize = 3;

	for (int row = 0; row < 5; ++row) {
		for (int col = 0; col < 5; ++col) {
			const int px = originX + col * cellSize;
			const int py = originY + row * cellSize;
			const uint32 color = ((row + col) & 1) ? 8 : 1;
			s.fillRect(Common::Rect(px, py, px + cellSize, py + cellSize), color);
		}
	}

	// Center marker (party position in the 5x5 area window).
	const int markX = originX + 2 * cellSize;
	const int markY = originY + 2 * cellSize;
	s.fillRect(Common::Rect(markX, markY, markX + cellSize, markY + cellSize),
		15);

	const Common::String anchorStr = Common::String::format("%u,%u",
		(unsigned)anchorX, (unsigned)anchorY);
	s.writeStringC(3, 16, 10, anchorStr);
}

void InGameMainScreenDialog::drawWildernessPositionMarker(Surface &s,
		uint8 mapType, uint8 wildernessX, uint8 wildernessY) {
	const int type = (int)mapType;
	const int xOffset = (type >= 0 && type < 8)
		? kWildernessMapXOffsets[type]
		: 0;
	const int shiftedX = xOffset + (int)wildernessX;

	// GFX_update3D bucketing:
	//   xBucket: <2 => raw, 2..41 => 2, 42 => 3, >=43 => 4
	int xBucket = shiftedX;
	if (xBucket > 1) {
		if (xBucket < 42)
			xBucket = 2;
		else if (xBucket < 43)
			xBucket = 3;
		else
			xBucket = 4;
	}

	// yBucket: <2 => raw, 2..33 => 2, 34 => 3, >=35 => 4
	int yBucket = (int)wildernessY;
	if (yBucket < 2)
		yBucket = (int)wildernessY;
	else if (yBucket < 34)
		yBucket = 2;
	else if (yBucket < 35)
		yBucket = 3;
	else
		yBucket = 4;

	// Left panel 5x5 block uses 3x3 pixels per cell from (3,3) char origin.
	const int originX = 3 * 8 + 1;
	const int originY = 3 * 8 + 1;
	const int px = originX + clampInt(xBucket, 0, 4) * 3;
	const int py = originY + clampInt(yBucket, 0, 4) * 3;

	// Simple 6-frame pulse marker (legacy had a 6-step animated position tile).
	const uint32 markerColor = (_wildernessAnimFrame < 3) ? 15 : 14;
	s.fillRect(Common::Rect(px, py, px + 3, py + 3), markerColor);

	_wildernessAnimFrame = static_cast<uint8>((_wildernessAnimFrame + 1) % 6);
}

void InGameMainScreenDialog::drawMap3dIfNeeded(Surface &s) {
	if (_mode != kModeDungeon && _mode != kModeWilderness && _mode != kModeCombat)
		return;

	if (!::Goldbox::Poolrad::g_engine)
		return;

	::Goldbox::RuntimeMapSnapshot snapshot;
	const RuntimeExchange *exchange =
		::Goldbox::Poolrad::g_engine->getRuntimeExchange();
	if (!exchange || !exchange->captureMapSnapshot(snapshot) || !snapshot.valid)
		return;

	uint16 mapX = snapshot.dungeonX;
	uint16 mapY = snapshot.dungeonY;
	uint8 mapDir = snapshot.dungeonDir;
	const uint8 effectiveType = effectiveMapTypeFromRuntime();

	if (effectiveType == 1) {
		Data::DaxBlockGeo *geo = ::Goldbox::Poolrad::g_engine->getActiveGeoBlock();
		if (!geo)
			return;

		Gfx::FirstPersonRenderer::draw3dWorld(&s, mapDir % 8,
			(int)mapX, (int)mapY, *geo,
			::Goldbox::Poolrad::g_engine->getWalldefSlotCache());
		return;
	}

	if (effectiveType > 1 && effectiveType < 5) {
		const int wildX = static_cast<int>(snapshot.wildernessX);
		const int wildY = static_cast<int>(snapshot.wildernessY);
		const int mapType = (int)effectiveType;
		const int xOffset = (mapType >= 0 && mapType < 8)
			? kWildernessMapXOffsets[mapType]
			: 0;

		const uint8 anchorX = (uint8)clampInt(wildX + xOffset - 2, 0, 39);
		const uint8 anchorY = (uint8)clampInt(wildY - 2, 0, 31);
		drawWildernessAreaBlock(s, anchorX, anchorY);
		drawWildernessPositionMarker(s, effectiveType,
			(uint8)clampInt(wildX, 0, 255),
			(uint8)clampInt(wildY, 0, 255));
	}
}

void InGameMainScreenDialog::draw() {
	if (!_isVisible)
		return;

	if (_mode == kModeNone) {
		drawWindow(1, 1, 38, 22);
		return;
	}

	const bool showMiniWindow = (_mode != kModeWilderness);
	drawMainScreenWindows(showMiniWindow);

	Surface s = getSurface();
	drawMap3dIfNeeded(s);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

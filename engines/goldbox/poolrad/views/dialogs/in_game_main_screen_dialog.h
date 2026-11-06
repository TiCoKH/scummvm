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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_MAIN_SCREEN_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_MAIN_SCREEN_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/gfx/combat_tile_cache.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class InGameMainScreenDialog : public Dialog {
public:
	enum DrawMode {
		kModeNone = 0,
		kModeShop,
		kModeCamping,
		kModeDungeon,
		kModeWilderness,
		kModeAfterCombat,
		kModeCombat
	};

private:
	DrawMode _mode = kModeNone;
	uint8 _wildernessAnimFrame = 0;

	// GB13-024: WILDCOM terrain-tile cache for the 5x5/24x24 map window.
	// Loaded lazily (WILDCOM 0-33 + RANDCOM 0-5) the first time the
	// wilderness map is drawn; shared across redraws.
	::Goldbox::Gfx::CombatTileCache _wildernessTiles;

	uint8 effectiveMapTypeFromRuntime() const;
	void drawMap3dIfNeeded(Surface &s);
	// origin*: clamped 5x5 map-window origin in ATLAS coordinates (already
	// region-offset). party*: the party's atlas cell (unclamped), for the
	// independently-computed marker.
	void drawWildernessAreaBlock(Surface &s, int originAtlasX, int originAtlasY);
	void drawWildernessPositionMarker(Surface &s, int prospectiveOriginX,
		int prospectiveOriginY, int partyAtlasX, int partyAtlasY);
	void ensureWildernessTilesLoaded();

public:
	InGameMainScreenDialog(const Common::String &name = "InGameMainScreen");
	~InGameMainScreenDialog() override {}

	void setDrawMode(DrawMode mode);
	void draw() override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_MAIN_SCREEN_DIALOG_H

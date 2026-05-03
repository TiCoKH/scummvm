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

#include "common/str.h"
#include "goldbox/engine.h"
#include "goldbox/data/player_character.h"
#include "goldbox/poolrad/views/in_game_view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// Direction names indexed by 8-direction value (0-7).
static const char *const kDirNames[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

// Movement delta tables indexed by 4-direction (0=N, 1=E, 2=S, 3=W).
static const int kDx[4] = {0, 1, 0, -1};
static const int kDy[4] = {-1, 0, 1, 0};

// Party-summary column positions (right panel, 40-col grid).
static const int kPartyColName  = 17;
static const int kPartyColAC    = 33;
static const int kPartyColHP    = 36;
static const int kPartyRowHdr   =  2;
static const int kPartyRowStart =  4;

// Position / status row.
static const int kStatusRow = 15;
static const int kStatusCol = 17;

// -----------------------------------------------------------------------

InGameView::InGameView() : View("InGame") {
}

// -----------------------------------------------------------------------
// Public interface

void InGameView::onEnter(GameState state) {
	applyScreenByState(state);
}

void InGameView::applyScreenByState(GameState state) {
	switch (state) {
	case GS_DUNGEON_MAP:
		_mode = kModeDungeon;
		break;
	case GS_WILDERNESS_MAP:
		_mode = kModeWilderness;
		break;
	case GS_SHOP:
		_mode = kModeShop;
		break;
	case GS_CAMPING:
		_mode = kModeCamping;
		break;
	case GS_AFTER_COMBAT:
		_mode = kModeAfterCombat;
		break;
	case GS_COMBAT:
		_mode = kModeCombat;
		break;
	default:
		_mode = kModeNone;
		break;
	}
	redraw();
}

// -----------------------------------------------------------------------
// View lifecycle

bool InGameView::msgFocus(const FocusMessage &msg) {
	View::msgFocus(msg);
	redraw();
	return true;
}

bool InGameView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

// -----------------------------------------------------------------------
// Drawing

void InGameView::draw() {
	switch (_mode) {
	case kModeDungeon:
		drawDungeonMode();
		break;
	case kModeWilderness:
		drawWildernessMode();
		break;
	case kModeShop:
		drawShopMode();
		break;
	case kModeCamping:
		drawCampingMode();
		break;
	case kModeAfterCombat:
		drawAfterCombatMode();
		break;
	default:
		break;
	}
}

void InGameView::drawDungeonMode() {
	// w_inner_frame = true for dungeon (GS_DUNGEON_MAP).
	drawMainScreenWindows(true);
	Surface s = getSurface();
	drawPartySummary(s);
	drawPositionTime(s);
	// TODO: Render 3D forward view via GFX_renderWallRun once geo/colors ready.
}

void InGameView::drawWildernessMode() {
	// w_inner_frame = false for wilderness (GS_WILDERNESS_MAP).
	drawMainScreenWindows(false);
	Surface s = getSurface();
	drawPartySummary(s);
	drawPositionTime(s);
	// TODO: Draw area-map block and wilderness position tile.
}

void InGameView::drawShopMode() {
	// w_inner_frame = true for shop (GS_SHOP).
	drawMainScreenWindows(true);
	// TODO: Blit NPC portrait at (3,3) and shop dialog content.
}

void InGameView::drawCampingMode() {
	// w_inner_frame = true for camping (GS_CAMPING).
	drawMainScreenWindows(true);
	// TODO: Draw camp/rest menu state area.
}

void InGameView::drawAfterCombatMode() {
	// w_inner_frame = true for after-combat (GS_AFTER_COMBAT).
	drawMainScreenWindows(true);
	// TODO: Draw loot and post-combat summary panel.
}

void InGameView::drawPartySummary(Surface &s) {
	// Header row.
	s.writeStringC(kPartyColName, kPartyRowHdr, 15, "Name");
	s.writeStringC(kPartyColAC,   kPartyRowHdr, 15, "AC  HP");

	int row = kPartyRowStart;
	const Common::Array<Data::PlayerCharacter *> &party = g_engine->getParty();
	for (uint i = 0; i < party.size() && row < 14; ++i) {
		const Data::PlayerCharacter *pc = party[i];
		if (!pc)
			continue;

		// Name (truncated to 15 chars to stay inside the right panel).
		Common::String nameStr = pc->name;
		if (nameStr.size() > 15)
			nameStr = nameStr.substr(0, 15);
		s.writeStringC(kPartyColName, row, 10, nameStr);

		// AC (3 chars left-justified).
		int ac = pc->armorClass.getCurrent();
		Common::String acStr = Common::String::format("%-3d", ac);
		s.writeStringC(kPartyColAC, row, 10, acStr);

		// HP (right-aligned up to 3 digits).
		Common::String hpStr = Common::String::format("%d", (int)pc->hitPoints.current);
		int hpCol = kPartyColHP + (3 - (int)hpStr.size());
		s.writeStringC(hpCol, row, 10, hpStr);

		++row;
	}

	// Clear any leftover lines from a previously longer party list.
	for (; row < 14; ++row)
		s.clearBox(kPartyColName, row, 38, row, 0);
}

void InGameView::drawPositionTime(Surface &s) {
	// Format: "X,Y DIRECTION [search]"
	Common::String posStr = Common::String::format("%d,%d %s",
		(int)_mapX, (int)_mapY, kDirNames[_mapDir]);

	if (_searchMode)
		posStr += " search";

	s.clearBox(kStatusCol, kStatusRow, 38, kStatusRow, 0);
	s.writeStringC(kStatusCol, kStatusRow, 10, posStr);
}

// -----------------------------------------------------------------------
// Input

bool InGameView::msgKeypress(const KeypressMessage &msg) {
	switch (_mode) {
	case kModeDungeon:
		return handleDungeonKeypress(msg);
	default:
		return false;
	}
}

bool InGameView::handleDungeonKeypress(const KeypressMessage &msg) {
	bool handled = true;

	switch (msg.keycode) {
	// --- Movement (arrow keys) ---
	case Common::KEYCODE_UP:
		stepForward();
		break;
	case Common::KEYCODE_LEFT:
		_mapDir = (_mapDir + 6) % 8;
		break;
	case Common::KEYCODE_RIGHT:
		_mapDir = (_mapDir + 2) % 8;
		break;
	case Common::KEYCODE_DOWN:
		_mapDir = (_mapDir + 4) % 8;
		break;

	// --- Menu keys ---
	case Common::KEYCODE_a:
		// TODO: Toggle area-map overlay.
		break;
	case Common::KEYCODE_s:
		_searchMode = !_searchMode;
		break;
	case Common::KEYCODE_e:
		// TODO: Trigger encamp — orchestrator should dispatch ECL_ONREST.
		break;
	case Common::KEYCODE_v:
		// View character sheet.
		replaceView("ViewCharacter");
		return true;
	case Common::KEYCODE_c:
		// TODO: Open spell menu for selected player.
		break;
	case Common::KEYCODE_l:
		// Look / search location — set flag; orchestrator dispatches ECL_ONSEARCH.
		_searchMode = true;
		break;

	default:
		handled = false;
		break;
	}

	if (handled)
		redraw();

	return handled;
}

void InGameView::stepForward() {
	// Convert 8-direction to 4-direction index (N=0, E=1, S=2, W=3).
	int dir4 = _mapDir / 2;
	int newX = (int)_mapX + kDx[dir4];
	int newY = (int)_mapY + kDy[dir4];

	// Boundary check: GEO maps are 16×16.
	if (newX < 0 || newX >= 16 || newY < 0 || newY >= 16)
		return;

	// TODO: Check wall / door collision via DaxBlockGeo once geo is loaded.
	_mapX = (uint16)newX;
	_mapY = (uint16)newY;
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

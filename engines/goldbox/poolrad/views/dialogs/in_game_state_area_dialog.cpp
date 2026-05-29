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
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/views/dialogs/in_game_state_area_dialog.h"
#include "goldbox/runtime/runtime_exchange.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

// 8-way compass directions indexed 0-7.
static const char *const kDirNames8[8] = {
    "N", "NE", "E", "SE", "S", "SW", "W", "NW"
};

// Convert cardinal direction (0=N,1=E,2=S,3=W) to 8-way index
// (0=N,2=E,4=S,6=W).
static uint8 cardinalTo8Way(uint8 cardinal) {
    return static_cast<uint8>((cardinal & 0x03) * 2);
}

static int wildernessXOffsetForType(uint8 mapType) {
    if (mapType == 3)
        return 13;
    if (mapType == 4)
        return 26;
    return 0;
}

static Common::String twoDigits(int value) {
    return Common::String::format("%02d", value);
}

InGameStateAreaDialog::InGameStateAreaDialog(const Common::String &name)
    : Dialog(name) {
    // Matches right-side state/status area used by DIALOG_StateArea.
    setBounds(Window(17, 15, 38, 15));
}

void InGameStateAreaDialog::setState(Goldbox::GameState state) {
    _state = state;
    redraw();
}

void InGameStateAreaDialog::setStateFromSnapshot(
        const Goldbox::RuntimeMapSnapshot &snapshot) {
    _state = snapshot.gameState;
    redraw();
}

void InGameStateAreaDialog::draw() {
    if (!_isVisible)
        return;

    Surface s = getSurface();

    int posX = 0;
    int posY = 0;
    uint8 mapDir = _mapDir;
    int hour = 0;
    int minute = 0;
    bool hideCoords = false;
    bool runtimeSearch = false;
    bool isDungeon = true;
    Goldbox::GameState gameState = _state;

    if (::Goldbox::Poolrad::g_engine) {
        const RuntimeExchange *exchange =
            ::Goldbox::Poolrad::g_engine->getRuntimeExchange();
        ::Goldbox::RuntimeMapSnapshot snapshot;
        if (exchange && exchange->captureMapSnapshot(snapshot)
                && snapshot.valid) {
            posX = static_cast<int>(snapshot.dungeonX);
            posY = static_cast<int>(snapshot.dungeonY);
            mapDir = snapshot.dungeonDir;

            hour = static_cast<int>(snapshot.clockHour);
            minute = static_cast<int>(snapshot.clockMinute);
            hideCoords = snapshot.hideCoords;
            runtimeSearch = snapshot.searchActive;
            gameState = snapshot.gameState;

            uint8 mapType = snapshot.mapType;
            if (snapshot.indoorMode)
                mapType = 1;

            if (mapType > 1) {
                isDungeon = false;
                posX = static_cast<int>(snapshot.wildernessX)
                    + wildernessXOffsetForType(mapType);
                posY = static_cast<int>(snapshot.wildernessY);
            }
        }
    }

    _mapDir = mapDir;
    _isDungeon = isDungeon;

    // Convert direction to 8-way index for display.
    uint8 displayDir;
    if (_isDungeon)
        displayDir = cardinalTo8Way(_mapDir);
    else
        displayDir = _mapDir % 8;

    Common::String line;
    if (!hideCoords)
        line = Common::String::format("%d,%d ", posX, posY);

    line += kDirNames8[displayDir];
    line += " ";
    line += twoDigits(hour);
    line += ":";
    line += twoDigits(minute);

    if (gameState == GS_CAMPING)
        line += " CAMPING";
    else if (runtimeSearch)
        line += " SEARCH";

    if (line.size() > 22)
        line = line.substr(0, 22);

    s.clearBox(0, 0, 21, 3, 0);
    s.writeStringC(0, 0, 10, line);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

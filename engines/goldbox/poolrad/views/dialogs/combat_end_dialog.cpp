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

#include "goldbox/poolrad/views/dialogs/combat_end_dialog.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatEndDialog::CombatEndDialog()
        : Dialog("CombatEnd"), _isCombatEnd(false), _canSurrender(false),
          _awaitingInput(false) {
}

CombatEndDialog::~CombatEndDialog() {
}

void CombatEndDialog::prepare(bool isCombatEnd, bool canSurrender) {
    _isCombatEnd  = isCombatEnd;
    _canSurrender = canSurrender;
}

void CombatEndDialog::activate() {
    Dialog::activate();
    _awaitingInput = false;

    // Combat over or no surrender option: resolve immediately without input.
    if (_isCombatEnd || !_canSurrender) {
        postResult(!_isCombatEnd);
        return;
    }

    // Show surrender prompt and wait for Y/N.
    _awaitingInput = true;
}

void CombatEndDialog::deactivate() {
    _awaitingInput = false;
    Dialog::deactivate();
}

void CombatEndDialog::draw() {
    if (!_awaitingInput)
        return;
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    s.writeStringC(0, 24, 10, "Surrender? YES NO");
}

bool CombatEndDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_awaitingInput)
        return false;

    char ascii = msg.ascii;
    if (ascii >= 'a' && ascii <= 'z')
        ascii -= 32;

    if (ascii == 'Y' || msg.keycode == Common::KEYCODE_RETURN) {
        postResult(false); // surrendered → don't continue
        return true;
    }
    if (ascii == 'N' || msg.keycode == Common::KEYCODE_ESCAPE) {
        postResult(true);  // declined surrender → continue
        return true;
    }
    return true; // consume all keys while prompt is active
}

void CombatEndDialog::postResult(bool continueEncounter) {
    _awaitingInput = false;
    deactivate();
    // Post to "Combat" (CombatView) with PA_NONE intValue as sentinel,
    // using _success to carry continueEncounter.
    g_events->postMenuResult("Combat", continueEncounter,
        Common::KEYCODE_INVALID, -1, Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

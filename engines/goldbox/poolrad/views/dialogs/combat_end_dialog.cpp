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
        : Dialog("CombatEnd"), _continueYesNo(nullptr) {
    setBounds(Window(0, 24, 39, 24));

    HorizontalYesNoConfig cfg;
    cfg.promptTxt       = "Continue Battle? ";
    cfg.promptColor     = 15;
    cfg.textColor       = 10;
    cfg.selectColor     = 15;
    cfg.backgroundColor = 8;
    _continueYesNo = new HorizontalYesNo("CombatEndYesNo", cfg);
    _continueYesNo->setParent(this);
}

CombatEndDialog::~CombatEndDialog() {
    delete _continueYesNo;
}

void CombatEndDialog::prepare() {
    if (_continueYesNo->isActive())
        _continueYesNo->deactivate();
}

void CombatEndDialog::activate() {
    Dialog::activate();
    _continueYesNo->activate();
}

void CombatEndDialog::deactivate() {
    if (_continueYesNo->isActive())
        _continueYesNo->deactivate();
    Dialog::deactivate();
}

void CombatEndDialog::draw() {
    if (!_isVisible)
        return;
    _continueYesNo->setRedraw();
    _continueYesNo->draw();
}

bool CombatEndDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;
    return _continueYesNo->msgKeypress(msg);
}

void CombatEndDialog::handleMenuResult(const MenuResultMessage &result) {
    if (!result._hasIntValue || !_continueYesNo->isActive())
        return;

    // HorizontalYesNo posts intValue 1 for YES and 0 for NO.
    postResult(result._intValue == 1);
}

void CombatEndDialog::postResult(bool continueEncounter) {
    deactivate();
    // Post to "Combat" (CombatView) with -1 as sentinel, using _success to
    // carry continueEncounter.
    g_events->postMenuResult("Combat", continueEncounter,
        Common::KEYCODE_INVALID, -1, Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

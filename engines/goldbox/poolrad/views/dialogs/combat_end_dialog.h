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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_END_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_END_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * "Continue Battle?" prompt shown at the end of a round when the party won
 * (mirrors the DIALOG_InputYesNo call in DIALOG_CombatEnd).
 *
 * Wraps a HorizontalYesNo. YES keeps the encounter going (posts
 * _success=true); NO ends it (_success=false). Posts a MenuResultMessage to
 * "Combat" with intValue -1 as a sentinel, handled by
 * CombatView::handleMenuResult.
 */
class CombatEndDialog : public Dialog {
public:
    CombatEndDialog();
    ~CombatEndDialog() override;

    /** Prepare before activation. */
    void prepare();

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    HorizontalYesNo *_continueYesNo;

    void postResult(bool continueEncounter);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_END_DIALOG_H

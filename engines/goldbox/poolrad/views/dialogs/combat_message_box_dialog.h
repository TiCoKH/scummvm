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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MESSAGE_BOX_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MESSAGE_BOX_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/message_box_api.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data

namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Right-hand combat panel (cols 23-38, rows 1-21).
 *
 * In the original both the focused actor block (COMBATVIEW_drawCombatInfo)
 * and the combat messages are written with TEXT_drawIntoMsgBox into this
 * same window, so this dialog owns both:
 * - rows 1-9: actor name, hit points, AC, weapon/status
 * - message rows: GameText combat placement (default row 10)
 *
 * Messages are rendered synchronously because the combat loop blocks.
 * Not attached to the UI tree: CombatView draws it in a fixed order.
 */
class CombatMessageBoxDialog : public Dialog, public MessageBoxApi {
public:
    static const int kLeft = 23;
    static const int kTop = 1;
    static const int kRight = 38;
    static const int kBottom = 21;
    static const int kInfoBottom = 9;
    static const int kTextColor = 10;
    static const uint8 kBackgroundColor = 8;
    static const uint8 kDefaultMessageRow = 10;

    CombatMessageBoxDialog();
    ~CombatMessageBoxDialog() override {}

    /** Actor shown in the info block on every draw(); nullptr hides it. */
    void setActor(::Goldbox::Data::PlayerCharacter *ch) { _actor = ch; }
    ::Goldbox::Data::PlayerCharacter *getActor() const { return _actor; }

    /** Draw the info block for ch now (COMBATVIEW_drawCombatInfo). */
    void drawActorInfo(::Goldbox::Data::PlayerCharacter *ch);

    // MessageBoxApi
    void showMessage(::Goldbox::Data::PlayerCharacter *speaker,
                     const Common::String &text, uint8 row,
                     bool timed) override;
    void clearMessage() override;
    bool isBusy() const override;

    void draw() override;

private:
    ::Goldbox::Data::PlayerCharacter *_actor = nullptr;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MESSAGE_BOX_DIALOG_H

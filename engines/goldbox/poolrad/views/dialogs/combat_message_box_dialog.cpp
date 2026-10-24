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

#include "goldbox/poolrad/views/dialogs/combat_message_box_dialog.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/player_character.h"
#include "goldbox/engine.h"
#include "goldbox/gfx/game_text.h"
#include "goldbox/gfx/surface.h"
#include "common/system.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatMessageBoxDialog::CombatMessageBoxDialog()
        : Dialog("CombatMessageBox") {
}

void CombatMessageBoxDialog::drawActorInfo(
        ::Goldbox::Data::PlayerCharacter *ch) {
    if (!ch)
        return;

    Surface s = getSurface();

    // Clear only the static info area (rows 1-9); the rows below are the
    // GameText combat message area and must not be clobbered here.
    s.clearBox(kLeft, kTop, kRight, kInfoBottom, kBackgroundColor);

    // Row 1: character name.
    s.writeStringC(kLeft, 1, kTextColor, ch->name);

    // Row 3: Hitpoints label + value.
    s.writeStringC(kLeft, 3, kTextColor, "Hitpoints");
    s.writeStringC(33, 3, kTextColor, Common::String::format("%d/%d",
        ch->hitPoints.current, ch->hitPoints.max));

    // Row 5: Armor class label + value.
    s.writeStringC(kLeft, 5, kTextColor, "AC");
    s.writeStringC(26, 5, kTextColor, Common::String::format("%d",
        ch->armorClass.getCurrent()));

    // BYTE_CHAR_POS_Y = 5 (AC row). Status anchors to row 7 (BYTE_CHAR_POS_Y + 2).
    static const int kStatusRow = 7;

    // Rows 7-8: equipped weapon display text (wraps up to 2 rows).
    const ::Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<const ::Goldbox::Data::ADnDCharacter *>(ch);
    const ::Goldbox::Data::Items::CharacterItem *weapon = adnd ?
        adnd->getEquippedItem(::Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon) {
        // getListDisplayText(false) mirrors ITEM_buildListDisplayText(..., false, false).
        const Common::String weaponText = weapon->getListDisplayText(false);
        // First line at row 7, overflow at row 8 — mirrors TEXT_BlockPrint rect {23,7,38,9}.
        const uint kMaxCols = kRight - kLeft + 1;
        if (weaponText.size() <= kMaxCols) {
            s.writeStringC(kLeft, kStatusRow, kTextColor, weaponText);
        } else {
            // Split at last word-break at or before kMaxCols.
            uint split = kMaxCols;
            while (split > 0 && !GameText::isWordBreak(weaponText[split - 1]))
                --split;
            if (split == 0)
                split = kMaxCols; // no word-break found, hard-split
            s.writeStringC(kLeft, kStatusRow, kTextColor,
                           weaponText.substr(0, split));
            s.writeStringC(kLeft, kStatusRow + 1, kTextColor,
                           weaponText.substr(split));
        }
    }

    // Row 7: status/condition (overlays weapon area when character is disabled/helpless).
    if (!ch->enabled) {
        s.writeStringC(kLeft, kStatusRow, kTextColor, ch->getStatusName());
    } else if (ch->hasNegativeEffect()) {
        s.writeStringC(kLeft, kStatusRow, kTextColor, "(Helpless)");
    }
}

void CombatMessageBoxDialog::showMessage(
        ::Goldbox::Data::PlayerCharacter *speaker,
        const Common::String &text, uint8 row, bool timed) {
    if (!g_engine)
        return;

    GameText &gameText = g_engine->getGameText();
    gameText.showMessage(speaker, text, row, GameText::COMBAT_MESSAGE, timed);
    while (gameText.advance())
        ;
    Surface s = getSurface();
    gameText.draw(s);
    g_system->updateScreen();
}

void CombatMessageBoxDialog::clearMessage() {
    if (g_engine)
        g_engine->getGameText().clearMessageArea();
}

bool CombatMessageBoxDialog::isBusy() const {
    return g_engine && g_engine->getGameText().isBusy();
}

void CombatMessageBoxDialog::draw() {
    if (_actor)
        drawActorInfo(_actor);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

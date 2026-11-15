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
#include "goldbox/gfx/backend/gfx_backend.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/player_character.h"
#include "goldbox/engine.h"
#include "goldbox/gfx/game_text.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/poolrad/data/poolrad_effects.h"
#include "goldbox/vm_interface.h"
#include "common/system.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatMessageBoxDialog::CombatMessageBoxDialog()
        : Dialog("CombatMessageBox") {
}

void CombatMessageBoxDialog::drawActorInfo(
        ::Goldbox::Data::PlayerCharacter *ch, bool clearPanel) {
    if (!ch)
        return;

    Surface s = getSurface();

    // CombatPrintCreatureInfo clears the whole panel. Plain redraws keep the
    // message rows, which are not repainted from GameText.
    if (clearPanel) {
        clearMessage();
        s.clearBox(kLeft, kTop, kRight, kBottom, ::Goldbox::Gfx::combatBackgroundColor());
    } else {
        s.clearBox(kLeft, kTop, kRight, kInfoBottom, ::Goldbox::Gfx::combatBackgroundColor());
    }

    // DisplayCreatureName
    int nameColor = kPartyNameColor;
    if (!ch->enabled)
        nameColor = kDisabledNameColor;
    else if (ch->combatSide == ::Goldbox::Data::CS_ENEMY)
        nameColor = kEnemyNameColor;
    s.writeStringC(kLeft, 1, nameColor, ch->name);

    // PrintHitPoints: current only, yellow when injured.
    s.writeStringC(kLeft, 3, kTextColor, VmInterface::getString("combat.hitpoints"));
    const int hpColor = ch->hitPoints.current < ch->hitPoints.max
        ? kInjuredColor : kTextColor;
    s.writeStringC(33, 3, hpColor,
        Common::String::format("%d", ch->hitPoints.current));

    s.writeStringC(kLeft, 5, kTextColor, "AC");
    s.writeStringC(26, 5, kTextColor, Common::String::format("%d",
        ch->armorClass.getCurrent()));

    // The status line goes two rows below the last row used (AC or weapon).
    int lastRow = 5;

    const ::Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<const ::Goldbox::Data::ADnDCharacter *>(ch);
    const ::Goldbox::Data::Items::CharacterItem *weapon = adnd ?
        adnd->getEquippedItem(::Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon)
        lastRow = drawWrapped(s, kWeaponTop, kWeaponBottom,
                              weapon->getListDisplayText(false));

    const int statusRow = lastRow + 2;
    if (!ch->enabled) {
        s.writeStringC(kLeft, statusRow, kStatusColor,
            VmInterface::getString(Common::String::format(
                "stats.conditions.%d", ch->healthStatus)));
    } else if (isHelpless(ch)) {
        s.writeStringC(kLeft, statusRow, kStatusColor, VmInterface::getString("combat.helpless"));
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

// DisplayMessageWindow: word wrap within cols kLeft-kRight; returns the last
// row written. Text past bottomRow is dropped instead of paging.
int CombatMessageBoxDialog::drawWrapped(Surface &s, int topRow, int bottomRow,
        const Common::String &text) {
    int col = kLeft;
    int row = topRow;
    uint pos = 0;
    while (pos < text.size()) {
        const uint end = GameText::wordEnd(text, pos, text.size());
        const int len = (int)(end - pos);
        if (col != kLeft && col + len > kRight + 1) {
            if (row == bottomRow)
                break;
            ++row;
            col = kLeft;
            while (pos < text.size() && text[pos] == ' ')
                ++pos;
            continue;
        }
        for (uint i = pos; i < end && col <= kRight; ++i)
            s.writeCharC(col++, row, kTextColor, text[i]);
        pos = end;
    }
    return row;
}

// IsHelpless: any of the helplessFx effects.
bool CombatMessageBoxDialog::isHelpless(::Goldbox::Data::PlayerCharacter *ch) {
    using namespace ::Goldbox::Data::Effects;
    static const uint8 kHelplessEffects[] = {
        E_POOLRAD_SNAKE_CHARM, E_POOLRAD_PARALYSIS, E_POOLRAD_SLEEP,
        E_POOLRAD_HELPLESS
    };
    const CharacterEffects *fx = ch->getEffects();
    if (!fx)
        return false;
    for (uint i = 0; i < ARRAYSIZE(kHelplessEffects); ++i)
        if (fx->hasEffect(kHelplessEffects[i]))
            return true;
    return false;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

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

#include "goldbox/poolrad/views/dialogs/treasure_dialog.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

static const uint8 SPELL_DETECT_MAGIC = 5;
static const uint8 SPELL_KNOW_ALIGNMENT = 11;
static const uint8 MAX_MEMORIZED_SPELLS = 21;

static ShopBaseConfig makeTreasureConfig() {
    ShopBaseConfig cfg;
    cfg.type = SHOP_TREASURE;
    cfg.menuPrompt = "";
    cfg.exitConfirmLine1 = "There is still treasure left.";
    cfg.exitConfirmLine2 = "Do you want to go back and claim it?";
    return cfg;
}

TreasureDialog::TreasureDialog(const Common::String &name)
    : ShopBaseDialog(name, makeTreasureConfig()) {
}

bool TreasureDialog::hasDetectSpell() const {
    // Scan selected character's memorized spells for Detect Magic or
    // Know Alignment (matches original scan in DIALOG_Treasure).
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch)
        return false;

    bool hasItems = false;
    bool hasMoney = false;
    const_cast<TreasureDialog *>(this)->getShopFlags(hasItems, hasMoney);
    if (!hasItems)
        return false;

    for (uint8 i = 0; i < MAX_MEMORIZED_SPELLS; ++i) {
        uint8 spellId = ch->spells.memorizedSpells[i];
        if (spellId == SPELL_DETECT_MAGIC || spellId == SPELL_KNOW_ALIGNMENT) {
            const_cast<TreasureDialog *>(this)->_detectSpellType = spellId;
            return true;
        }
    }
    return false;
}

void TreasureDialog::actionDetect() {
    // TODO: ACTION_UseSpell(_detectSpellType, false, false, ...)
}

void TreasureDialog::getShopFlags(bool &hasItems, bool &hasMoney) {
    // TODO: Query treasure pool state from engine/VM runtime.
    hasItems = false;
    hasMoney = false;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

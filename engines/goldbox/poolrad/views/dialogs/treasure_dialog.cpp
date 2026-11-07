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
#include "goldbox/poolrad/noncombat_spell_cast.h"
#include "goldbox/spells/spell_casting.h"
#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_handlers.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"
#include "common/debug.h"

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
    getTreasurePoolFlags(hasMoney, hasItems);
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
    // Resolve the selected character the same way hasDetectSpell() does; the
    // menu only offers Detect when hasDetectSpell() returned true, so a valid
    // caster with a memorized spell id 5/11 is expected here.
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch || !Goldbox::Spells::g_spellCasting)
        return;
    if (_detectSpellType != SPELL_DETECT_MAGIC &&
            _detectSpellType != SPELL_KNOW_ALIGNMENT)
        return;

    // The raw memorized id maps DIRECTLY to the Spells enum: id 5 ==
    // SP_CL1_DETECT_MAGIC, id 11 == SP_MUL1_DETECT_MAGIC (data/spells/spell.h).
    // (Know Alignment id 11 shares the Mage Detect Magic slot in the legacy
    // detect scan.) No table lookup is needed.
    const Goldbox::Data::Spells::Spells spell =
        static_cast<Goldbox::Data::Spells::Spells>(_detectSpellType);

    // Noncombat cast origin, identical to SpellBookDialog::castMemorizedSpell:
    // a memorized slot is spent (fromMagicItem false), never an item/temple
    // charge, and distinct from the combat path. castSpell consumes exactly
    // ONE memorized slot on CAST_OK and leaves it intact on CAST_ABORTED.
    Goldbox::Poolrad::NoncombatSpellCast cast(ch, false);
    const Goldbox::Spells::SpellCastResult result = cast.cast(spell);
    if (result.status != Goldbox::Spells::CAST_OK)
        return; // cancelled / failed: nothing consumed, no state change

    // Confirmed cast: change the detection display state and refresh WITHOUT
    // implicitly identifying every hidden property and WITHOUT rerolling the
    // generated items. We only flip the dialog's identify-display flag so the
    // item list re-renders with the Detect marker (getListDisplayText's
    // identifyActive path); the per-item `hidden` masks are left untouched.
    //
    // REVIEW-OPEN: the exact original "detection state" global (the loot
    // detect flag DIALOG_Treasure sets) could not be byte-confirmed. The
    // confirmable effects are implemented here: one spell consumed via the
    // noncombat path + display refresh with the Detect marker active. The
    // specific persistent detection-state bit is flagged for review.
    _detectActive = true;
    debug(3, "TreasureDialog::actionDetect: cast spell %u, detectActive=%d",
        (unsigned)_detectSpellType, (int)_detectActive);

    if (g_events)
        g_events->postEclStateMessage(EclVmMessage::ST_SCREEN_REFRESH, 0,
            EclVmMessage::VT_UINT8);
    redraw();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

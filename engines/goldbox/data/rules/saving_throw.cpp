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
 */

#include "goldbox/data/rules/saving_throw.h"

#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Data {
namespace Rules {

bool checkSavingThrow(Goldbox::Data::ADnDCharacter &character,
        Goldbox::Combat::CombatGlobals *combat,
        const Effects::EffectHandlerBase *handler,
        Effects::EffectHostBridge *bridge,
        Goldbox::Data::Spells::SaveVerseType saveType,
        int8 saveModifier) {
    if (!Goldbox::g_engine)
        return false;

    const int roll = Goldbox::g_engine->rollDice(1, 20);
    if (roll == 1)
        return false;
    if (roll == 20)
        return true;

    // Accumulate saving throw modifiers from ES_SAVING_THROW_MODS (set 12),
    // mirroring UTIL_checkEffectSet(12, char). Works on a copy so the live
    // combat globals (if any) are left untouched.
    Goldbox::Combat::CombatGlobals tempCombat;
    if (combat)
        tempCombat = *combat;
    tempCombat.savingThrowType = saveType;

    Effects::CharacterEffects *fx = character.getEffects();
    if (fx && handler) {
        Effects::EffectRuntime runtime(const_cast<Effects::EffectHandlerBase *>(handler), bridge);
        runtime.checkEffectSet(Effects::ES_SAVING_THROW_MODS, *fx, character, &tempCombat);
    }

    // SavingThrows fields are laid out in order matching saveType 0-4.
    const uint8 target = (&character.savingThrows.vsParalysis)[static_cast<uint8>(saveType)];
    const int total = roll + character.saveBonus + saveModifier + tempCombat.savingThrow;

    return total >= target;
}

} // namespace Rules
} // namespace Data
} // namespace Goldbox

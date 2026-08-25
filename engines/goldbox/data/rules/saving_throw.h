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

#ifndef GOLDBOX_DATA_RULES_SAVING_THROW_H
#define GOLDBOX_DATA_RULES_SAVING_THROW_H

#include "common/scummsys.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {

namespace Combat {
class CombatGlobals;
} // namespace Combat

namespace Data {

class ADnDCharacter;

namespace Effects {
class EffectHandlerBase;
class EffectHostBridge;
} // namespace Effects

namespace Rules {

/**
 * Shared saving throw check, extracted from the original per-game
 * static UTIL_CheckSavingThrow duplicate (see poolrad/effect_handler.cpp).
 *
 * Rolls 1d20: natural 1 always fails, natural 20 always succeeds.
 * Otherwise totals roll + saveBonus + saveModifier + any active
 * ES_SAVING_THROW_MODS effect modifier, compared against the character's
 * SavingThrows table entry for the given saveType.
 *
 * @param character    Target rolling the save.
 * @param combat       Live combat globals (may be null outside combat);
 *                     used only to seed the saveType for effect-set lookup.
 * @param handler      Effect handler used to evaluate ES_SAVING_THROW_MODS
 *                     (may be null to skip active-effect modifiers).
 * @param bridge       Host bridge forwarded to the effect-set evaluation.
 * @param saveType     Which saving throw column to check against.
 * @param saveModifier Additional flat modifier from the caller (spell/effect).
 * @return true if the saving throw succeeded.
 */
bool checkSavingThrow(Goldbox::Data::ADnDCharacter &character,
        Goldbox::Combat::CombatGlobals *combat,
        const Effects::EffectHandlerBase *handler,
        Effects::EffectHostBridge *bridge,
        Goldbox::Data::Spells::SaveVerseType saveType,
        int8 saveModifier);

} // namespace Rules
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_RULES_SAVING_THROW_H

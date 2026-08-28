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

#ifndef GOLDBOX_SPELLS_SPELL_DEFINITION_H
#define GOLDBOX_SPELLS_SPELL_DEFINITION_H

#include "common/str.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {
namespace Spells {

// A spell only needs a HandlerId beyond kHandlerGeneric when its behaviour
// cannot be derived purely from SpellEntry (saving throw + effectId),
// e.g. damage dice, HD-limited targeting, or non-effect side output.
enum HandlerId {
    kHandlerGeneric = 0,        // generic saving-throw + effect apply (default)
    kHandlerDetectMagicShared,
    kHandlerProtectionShared,
    kHandlerHoldPersonShared,
    kHandlerDispelMagicShared,
    kHandlerInvisibilityShared,
    kHandlerCureLightWounds,    // ID03: heal 1d8 HP
    kHandlerBurningHands,       // ID09: damage = caster level
    kHandlerCharmPerson,        // ID10: type/size check + charm effect
    kHandlerEnlarge,             // ID12: strength buff by caster level
    kHandlerReduce,              // ID13: saving throw; remove E_POOLRAD_ENLARGE_STRENGTHEN
    kHandlerFriends,             // ID14: charisma buff 2d4; power = old charisma
    kHandlerMagicMissile,        // ID15: damage = damageLevel + damageLevel*d4; behavior 8
    kHandlerShield,              // ID19: pure effect; no damage
    kHandlerShockingGrasp,       // ID20: damage = casterLevel + 1d8; behavior 12
    kHandlerSleep,              // ID21: shared 4d4 budget; filters targets by level cost
    kHandlerHoldPerson,          // ID23/ID49: save adjustment by target count; type/size check
    kHandlerResistFire,          // ID24: pure effect; delegates to generic path
    kHandlerSilence15Radius,     // ID25: pure effect; delegates to generic path
    kHandlerSlowPoison,          // ID26: status/effect gate; HP floor; effect transition
    kHandlerSnakeCharm,          // ID27: filters enemies by monsterType==0x0E and HP budget
    kHandlerSpiritualHammer,     // ID28: generic effect path + EFF_ADD on E_POOLRAD_SPIRITUAL_HAMMER
    kHandlerMirrorImage,         // ID32: effectPowerOverride = 1d4
    kHandlerStinkingCloud,       // ID34: creates persistent cloud object; applies E_STINKING_CLOUD_EXPAIR
    kHandlerStrength,            // ID35: bonus by class; exceptional str for fighters
    kHandlerAnimateDead,         // ID36: converts dead allies to undead combatants
    kHandlerCureBlindness        // ID37: removes E_POOLRAD_BLINDED; posts "can see" if removed
};

struct SpellDefinition {
    Goldbox::Data::Spells::Spells id;
    const Goldbox::Data::Spells::SpellEntry *entry;
    HandlerId handlerId;
    Common::String name;
    // Message posted to the host bridge when the effect lands on a target,
    // e.g. "is Blessed", "is Cursed". Empty string suppresses the message.
    Common::String effectMessage;

    SpellDefinition()
        : id(Goldbox::Data::Spells::SP_NONE), entry(nullptr),
        handlerId(kHandlerGeneric) {}

    SpellDefinition(Goldbox::Data::Spells::Spells spellId,
                    const Goldbox::Data::Spells::SpellEntry *spellEntry,
                    HandlerId handler,
                    const Common::String &spellName,
                    const Common::String &message = Common::String())
        : id(spellId), entry(spellEntry), handlerId(handler),
        name(spellName), effectMessage(message) {}
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_DEFINITION_H

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

#include "goldbox/spells/spell_registry.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/spells/spell_metadata.h"

namespace Goldbox {
namespace Spells {

SpellRegistry::SpellRegistry() {
    buildDefinitions();
}

const SpellDefinition *SpellRegistry::getDefinition(
        Goldbox::Data::Spells::Spells id) const {
    int key = static_cast<int>(id);
    if (!_definitions.contains(key))
        return nullptr;
    return &_definitions[key];
}

const ISpellHandler *SpellRegistry::getHandler(HandlerId id) const {
    int key = static_cast<int>(id);
    if (!_handlers.contains(key))
        return nullptr;
    return _handlers[key];
}

void SpellRegistry::setHandler(HandlerId id, ISpellHandler *handler) {
    int key = static_cast<int>(id);
    _handlers[key] = handler;
}

void SpellRegistry::setHandlerForSpell(Goldbox::Data::Spells::Spells id,
        HandlerId handler) {
    int key = static_cast<int>(id);
    if (!_definitions.contains(key))
        return;
    _definitions[key].handlerId = handler;
}

void SpellRegistry::buildDefinitions() {
    _definitions.clear();

    const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
        Goldbox::Data::Rules::getSpellEntries();

    for (uint i = 0; i < entries.size(); ++i) {
        Goldbox::Data::Spells::Spells spellId =
            static_cast<Goldbox::Data::Spells::Spells>(i);
        SpellDefinition def(spellId, &entries[i], kHandlerGeneric,
                            getSpellName(spellId),
                            effectMessageForSpell(spellId));
        _definitions[static_cast<int>(spellId)] = def;
    }
}

// static
Common::String SpellRegistry::effectMessageForSpell(
        Goldbox::Data::Spells::Spells id) {
    using namespace Goldbox::Data::Spells;
    // The msg each Magic handler passes to ResolveSpell (InitSpellFns).
    switch (id) {
    case SP_CL1_BLESS:            return "is Blessed";
    case SP_CL1_CURSE:            return "is Cursed";
    case SP_CL1_DETECT_MAGIC:
    case SP_MUL1_DETECT_MAGIC:
    case SP_MUL1_READ_MAGIC:
    case SP_CL2_FIND_TRAPS:
    case SP_CL2_SLOW_POISON:
    case SP_MUL2_DETECT_INVISIB:  return "is affected";
    case SP_CL1_PROT_FROM_EVIL:
    case SP_CL1_PROT_FROM_GOOD:
    case SP_MUL1_PROT_FROM_EVIL:
    case SP_MUL1_PROT_FROM_GOOD:
    case SP_MUL3_PROT_F_EVIL_10R:
    case SP_MUL3_PROT_F_GOOD_10R:
    case SP_MUL3_PROT_F_NORM_MSL: return "is protected";
    case SP_CL1_RESIST_COLD:      return "is cold-resistant";
    case SP_MUL1_CHARM_PERSON:
    case SP_CL2_SNAKE_CHARM:      return "is charmed";
    case SP_MUL1_FRIENDS:         return "is friendly";
    case SP_MUL1_SHIELD:          return "is shielded";
    case SP_MUL1_SLEEP:           return "falls asleep";
    case SP_CL2_HOLD_PERSON:
    case SP_MUL3_HOLD_PERSON:     return "is held";
    case SP_CL2_RESIST_FIRE:      return "is fire resistant";
    case SP_CL2_SILENCE_15R:      return "is silenced";
    case SP_MUL2_INVISIBILITY:
    case SP_MUL3_INVIS_10R:
    case SP_MI7:                  return "is invisible";
    case SP_MUL2_KNOCK:           return "Knock-Knock";
    case SP_MUL2_MIRROR_IMAGE:    return "is duplicated";
    case SP_MUL2_RAY_ENFEEBLE:    return "is weakened";
    case SP_CL3_CAUSE_BLINDNESS:  return "is blind";
    case SP_CL3_CAUSE_DISEASE:    return "is diseased";
    case SP_CL3_PRAYER:           return "is praying";
    case SP_CL3_BESTOW_CURSE:     return "has been cursed!";
    case SP_MUL3_BLINK:           return "is blinking";
    case SP_MUL3_HASTE:           return "is Hasted";
    case SP_MUL3_SLOW:            return "is Slowed";
    case SP_MI1:                  return "is Speedy";
    case SP_MI5:                  return "is paralyzed";
    case SP_MI11:                 return "is Reading";
    default:                      return Common::String();
    }
}

} // namespace Spells
} // namespace Goldbox

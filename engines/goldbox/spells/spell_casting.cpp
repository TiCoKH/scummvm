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

#include "goldbox/spells/spell_casting.h"
#include "goldbox/data/spells/spell_book.h"
#include "goldbox/data/effects/effect_system.h"

namespace Goldbox {
namespace Spells {

SpellCastingService::SpellCastingService()
    : _combatTargeter(nullptr), _nonCombatTargeter(nullptr) {
}

SpellCastResult SpellCastingService::castSpell(SpellContext &context,
        Goldbox::Data::Spells::Spells spell) {
    const SpellDefinition *definition = _registry.getDefinition(spell);
    if (!definition || !definition->entry)
        return SpellCastResult(CAST_ERROR);

    SpellCastResult ok = validate(context, *definition);
    if (ok.status != CAST_OK)
        return ok;

    ISpellTargeter *targeter = getTargeter(context);
    if (!targeter)
        return SpellCastResult(CAST_INVALID_TARGET);

    TargetSelection targets;
    if (!targeter->selectTarget(context, *definition, targets))
        return SpellCastResult(CAST_INVALID_TARGET);

    const ISpellHandler *handler = _registry.getHandler(definition->handlerId);
    if (!handler)
        handler = &_fallbackHandler;

    SpellCastResult result = handler->execute(context, *definition, targets);
    if (result.status == CAST_OK && context.effectSystem &&
            definition->entry->effectId != 0) {
        uint16 duration = definition->entry->fixedDuration;
        if (definition->entry->perLvlDuration != 0 && context.casterLevel > 0) {
            duration += (uint16)(definition->entry->perLvlDuration *
                    context.casterLevel);
        }
        uint8 power = 0xFF;
        bool immediate = true;

        if (!targets.targetCharacters.empty()) {
            for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
                Goldbox::Data::PlayerCharacter *target =
                    targets.targetCharacters[i];
                if (!target)
                    continue;
                Goldbox::Data::Effects::CharacterEffects *effects =
                    target->getEffects();
                if (!effects)
                    continue;
                context.effectSystem->applyEffect(*effects, *target,
                        definition->entry->effectId, duration, power,
                        immediate);
            }
        } else if (context.caster) {
            Goldbox::Data::Effects::CharacterEffects *effects =
                context.caster->getEffects();
            if (effects) {
                context.effectSystem->applyEffect(*effects, *context.caster,
                        definition->entry->effectId, duration, power, immediate);
            }
        }
    }
    if (result.status == CAST_OK && context.spellBook) {
        uint8 count = context.spellBook->getMemorized(spell);
        if (count > 0)
            context.spellBook->setMemorized(spell, count - 1);
    }
    return result;
}

void SpellCastingService::setCombatTargeter(ISpellTargeter *targeter) {
    _combatTargeter = targeter;
}

void SpellCastingService::setNonCombatTargeter(ISpellTargeter *targeter) {
    _nonCombatTargeter = targeter;
}

SpellCastResult SpellCastingService::validate(const SpellContext &context,
        const SpellDefinition &definition) const {
    if (!context.spellBook)
        return SpellCastResult(CAST_ERROR);

    if (!context.spellBook->isKnown(definition.id))
        return SpellCastResult(CAST_NOT_KNOWN);

    if (context.spellBook->getMemorized(definition.id) == 0)
        return SpellCastResult(CAST_NOT_MEMORIZED);

    return SpellCastResult(CAST_OK);
}

ISpellTargeter *SpellCastingService::getTargeter(
        const SpellContext &context) const {
    if (context.inCombat)
        return _combatTargeter;
    return _nonCombatTargeter;
}

} // namespace Spells
} // namespace Goldbox

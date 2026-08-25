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

#include "goldbox/spells/spell_targeter.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Spells {

namespace {

// Caps for AREA_TARGET_1..4 (see AreaOfEffect); other codes (LOS tile,
// diameter, level-scaled) need battlefield-map info the spells/ module
// intentionally doesn't depend on, so they are left uncapped for now.
int maxTargetsForArea(Goldbox::Data::Spells::AreaOfEffect area) {
    switch (area) {
    case Goldbox::Data::Spells::AREA_TARGET_1:
        return 1;
    case Goldbox::Data::Spells::AREA_TARGET_2:
        return 2;
    case Goldbox::Data::Spells::AREA_TARGET_3:
        return 3;
    case Goldbox::Data::Spells::AREA_TARGET_4:
        return 4;
    default:
        return -1; // uncapped by this helper
    }
}

// Mirrors SPELL_ApplyToTargetList's side selection: offensive spells reach
// for the opposing side, beneficial spells stay on the caster's own side.
const Common::Array<Goldbox::Data::PlayerCharacter *> &pickSidePool(
        const SpellContext &context, const SpellDefinition &definition) {
    return definition.entry->isOffensive ? context.enemies : context.allies;
}

// Shared area/list target build for ST_COMBAT and ST_TARGET_LIST.
bool buildSideTargetList(const SpellContext &context,
        const SpellDefinition &definition, TargetSelection &result) {
    const Common::Array<Goldbox::Data::PlayerCharacter *> &pool =
        pickSidePool(context, definition);

    const int cap = maxTargetsForArea(definition.entry->areaOfEffect);
    for (uint i = 0; i < pool.size(); ++i) {
        if (!pool[i] || !pool[i]->isAlive())
            continue;
        if (cap >= 0 && (int)result.targetCharacters.size() >= cap)
            break;
        result.targetCharacters.push_back(pool[i]);
    }
    return !result.targetCharacters.empty();
}

bool selectTargetsCommon(const SpellContext &context,
        const SpellDefinition &definition, TargetSelection &result) {
    if (!definition.entry)
        return false;

    switch (definition.entry->targetType) {
    case Goldbox::Data::Spells::ST_CASTER:
        if (!context.caster)
            return false;
        result.targetCharacters.push_back(context.caster);
        return true;

    case Goldbox::Data::Spells::ST_PARTY_MEMBER: {
        if (!context.targetPicker)
            return false;
        const Common::Array<Goldbox::Data::PlayerCharacter *> &pool =
            pickSidePool(context, definition);
        Goldbox::Data::PlayerCharacter *picked = nullptr;
        if (!context.targetPicker->pickSingleTarget(pool, picked) || !picked)
            return false;
        result.targetCharacters.push_back(picked);
        return true;
    }

    case Goldbox::Data::Spells::ST_COMBAT:
    case Goldbox::Data::Spells::ST_TARGET_LIST:
        return buildSideTargetList(context, definition, result);

    default:
        return false;
    }
}

} // namespace

bool CombatTargeter::selectTarget(const SpellContext &context,
        const SpellDefinition &definition, TargetSelection &result) {
    return selectTargetsCommon(context, definition, result);
}

bool NonCombatTargeter::selectTarget(const SpellContext &context,
        const SpellDefinition &definition, TargetSelection &result) {
    return selectTargetsCommon(context, definition, result);
}

} // namespace Spells
} // namespace Goldbox

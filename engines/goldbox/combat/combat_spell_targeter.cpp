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

#include "goldbox/combat/combat_spell_targeter.h"
#include "goldbox/combat/combat_ai.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/core/direction.h"
#include "goldbox/data/player_character.h"
#include "goldbox/spells/spell_duration.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Combat {

namespace {

struct Pick {
    Data::PlayerCharacter *target;
    TilePos tile;

    Pick() : target(nullptr), tile(0, 0) {}
};

const uint8 kAiSelectionAttempts = 20;

// Mirrors the AI branch of COMBAT_spellTargetSelection.
bool selectAutomatically(const Spells::SpellContext &ctx, CombatContext &cc,
        const Data::Spells::SpellEntry &entry, uint8 spellId, uint8 range,
        Pick &pick) {
    Data::PlayerCharacter *caster = ctx.caster;

    if (!entry.isOffensive) {
        pick.target = caster;
        if (spellId == Data::Spells::SP_CL1_CURE_LT_WOUNDS) {
            Data::PlayerCharacter *ally = nullptr;
            if (!findNearbyWoundedAlly(caster, cc, &ally) || !ally)
                return false;
            pick.target = ally;
        }
        pick.tile = cc.table.getCharacterPos(pick.target);
        return true;
    }

    if (!caster->combatState)
        return false;

    for (uint8 attempt = 0; attempt < kAiSelectionAttempts; ++attempt) {
        if (!cc.selectTargetForAction(caster, range, false, true))
            continue;

        Data::PlayerCharacter *candidate = caster->combatState->target;
        if (!candidate)
            continue;

        bool acceptable = true;
        const TilePos candidatePos = cc.table.getCharacterPos(candidate);

        // The burst around the candidate must not catch the caster's side.
        if (entry.minAITargets != 0) {
            cc.buildTargetListCore(candidatePos, 1, DIR_ANY,
                                   entry.minAITargets);
            for (uint i = 0; i < cc.targetList.entries.size(); ++i) {
                const Data::PlayerCharacter *other =
                    cc.table.getCharacter(cc.targetList.entries[i].idx);
                if (other && other->combatSide == caster->combatSide)
                    acceptable = false;
            }
        }

        // Do not stack a status effect on a target that already has one.
        if (candidate->hasNegativeEffect() &&
                Data::PlayerCharacter::isStatusEffectId(entry.effectId))
            acceptable = false;

        if (acceptable) {
            pick.target = candidate;
            pick.tile = candidatePos;
            return true;
        }
    }
    return false;
}

// Mirrors COMBAT_spellTargetSelection. On success the chosen tile is also
// stored in globals.targetPos (COMBAT_TARGET_X/Y).
bool selectOne(const Spells::SpellContext &ctx, CombatContext &cc,
        const Data::Spells::SpellEntry &entry, uint8 spellId, bool tileMode,
        Pick &pick) {
    const uint8 range = Spells::computeSpellRange(spellId, ctx.casterLevel,
        ctx.fromMagicItem);
    pick = Pick();

    bool ok;
    if (!ctx.aiControl && ctx.aimer) {
        ok = ctx.aimer->aim(ctx.caster, spellId, range, tileMode,
            pick.target, pick.tile);
        if (ok && ctx.caster->combatState)
            ctx.caster->combatState->target = pick.target;
    } else {
        ok = selectAutomatically(ctx, cc, entry, spellId, range, pick);
    }

    if (ok)
        cc.globals.targetPos = pick.tile;
    return ok;
}

// Collects every combatant within radius of pos (SPELL target list build).
void addTilesInRadius(CombatContext &cc, TilePos pos, uint8 radius,
        Spells::TargetSelection &result) {
    cc.buildTargetListCore(pos, 1, DIR_ANY, radius);
    for (uint i = 0; i < cc.targetList.entries.size(); ++i) {
        Data::PlayerCharacter *ch =
            cc.table.getCharacter(cc.targetList.entries[i].idx);
        if (ch)
            result.targetCharacters.push_back(ch);
    }
}

bool contains(const Common::Array<Data::PlayerCharacter *> &list,
        const Data::PlayerCharacter *ch) {
    for (uint i = 0; i < list.size(); ++i) {
        if (list[i] == ch)
            return true;
    }
    return false;
}

} // namespace

bool CombatSpellTargeter::selectTarget(const Spells::SpellContext &context,
        const Spells::SpellDefinition &definition,
        Spells::TargetSelection &result) {
    CombatContext *cc = VmInterface::getCombatContext();
    if (!cc || !context.caster || !definition.entry)
        return false;

    const Data::Spells::SpellEntry &entry = *definition.entry;
    const uint8 spellId = static_cast<uint8>(definition.id);
    const uint8 area = entry.areaOfEffect & 0x0F;

    Data::PlayerCharacter *caster = context.caster;
    const TilePos casterPos = cc->table.getCharacterPos(caster);
    cc->globals.targetPos = casterPos;
    cc->globals.multiTarget = false;

    // Players that cannot aim (no aimer) behave like the AI: no duplicate
    // prompt, so a repeated pick consumes an attempt.
    const bool interactive = !context.aiControl && context.aimer;

    if (area == 0) {
        result.targetCharacters.push_back(caster);
    } else if (area == 0x0F) {
        Pick pick;
        if (!selectOne(context, *cc, entry, spellId, false, pick))
            return false;

        if (!caster->combatState || !caster->combatState->target) {
            addTilesInRadius(*cc, cc->globals.targetPos, 0, result);
            cc->globals.multiTarget = true;
        } else {
            result.targetCharacters.push_back(caster->combatState->target);
        }
    } else if (area < 8 || area > 14) {
        int remaining = (entry.areaOfEffect & 3) + 1;
        while (remaining > 0) {
            Pick pick;
            if (!selectOne(context, *cc, entry, spellId, false, pick)) {
                --remaining;
                continue;
            }
            if (!contains(result.targetCharacters, pick.target)) {
                result.targetCharacters.push_back(pick.target);
                --remaining;
            } else if (!interactive) {
                --remaining;
            }
        }
        if (result.targetCharacters.empty())
            return false;
    } else {
        Pick pick;
        if (!selectOne(context, *cc, entry, spellId, true, pick))
            return false;
        addTilesInRadius(*cc, cc->globals.targetPos, entry.areaOfEffect & 7,
            result);
        cc->globals.multiTarget = true;
    }

    result.tileX = cc->globals.targetPos.col;
    result.tileY = cc->globals.targetPos.row;
    return true;
}

} // namespace Combat
} // namespace Goldbox

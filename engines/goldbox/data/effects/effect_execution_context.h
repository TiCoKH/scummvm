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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_EXECUTION_CONTEXT_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_EXECUTION_CONTEXT_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;

namespace Effects {

/**
 * Runtime context passed to effect trigger-set evaluation.
 *
 * This mirrors the original runtime model where many effect handlers depend on
 * combat/script context (attacker, target, damage flags, saving throw type,
 * etc.) and not only on passive effect state.
 */
struct EffectExecutionContext {
    // Primary actor being evaluated (usually attacker/defender/current PC).
    PlayerCharacter *actor;

    // Optional combat target relevant to current effect set.
    PlayerCharacter *target;

    // Optional source/caster that initiated the current action.
    PlayerCharacter *source;

    // Attack/damage context.
    uint8 attackRoll;
    uint8 attackCount;
    uint8 incomingDamage;

    // Legacy-style damage behavior flags (fire/cold/electric/magic/etc.).
    uint8 behaviorFlags;

    // Saving throw context.
    uint8 savingThrowCategory;
    int8 savingThrowBonus;

    // Script/action context.
    uint8 activeSpellId;
    bool inCombat;

    // Bookkeeping for trigger-run instrumentation.
    uint16 evaluatedEffects;

    EffectExecutionContext() : actor(nullptr), target(nullptr),
            source(nullptr), attackRoll(0), attackCount(0),
            incomingDamage(0), behaviorFlags(0), savingThrowCategory(0),
            savingThrowBonus(0), activeSpellId(0), inCombat(false),
            evaluatedEffects(0) {
    }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_EXECUTION_CONTEXT_H

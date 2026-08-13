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

#ifndef GOLDBOX_COMBAT_COMBAT_DAMAGE_H
#define GOLDBOX_COMBAT_COMBAT_DAMAGE_H

#include "common/scummsys.h"
#include "goldbox/combat/combat_globals.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
namespace Effects {
class EffectRuntime;
}
}

namespace Combat {

struct CombatContext;

/**
 * Result of applyDamage() — carries everything the UI layer needs to
 * build messages without re-querying character state after mutation.
 */
struct DamageResult {
    uint8 finalDamage;      // resolved damage amount (0 = nullified)
    bool spellLost;         // true if an in-progress spell was interrupted
    bool wentDown;          // true if character became disabled
    bool wasKilled;         // true if status is DEAD / GONE / STONED
    bool isDying;           // true if status is DYING (and !wasKilled)
    uint8 behaviorFlags;    // snapshot of globals.behaviorFlags after ETS_ON_DAMAGE_TAKEN

    DamageResult()
        : finalDamage(0), spellLost(false), wentDown(false),
          wasKilled(false), isDying(false), behaviorFlags(0) {}
};

/**
 * Pure data layer of COMBAT_ApplyDamageMessage.
 *
 * Applies ETS_ON_DAMAGE_TAKEN, calculates final damage, calls ch->damage(),
 * interrupts spell casting, and triggers ETS_ON_DEATH when the character
 * goes down. No UI output is performed here.
 *
 * @param ctx           Live combat context (globals, table, map).
 * @param ch            Target character.
 * @param baseDamage    Raw damage before modifier.
 * @param modifier      DAMAGE_NORMAL / DAMAGE_HALF / DAMAGE_NULLIFY.
 * @param applyModifier When false, modifier is ignored.
 * @param effectRuntime Effect runtime for trigger-set evaluation (may be null).
 * @return              DamageResult for the UI layer to consume.
 */
DamageResult applyDamage(CombatContext &ctx,
                         Data::PlayerCharacter *ch,
                         uint8 baseDamage,
                         DamageModifier modifier,
                         bool applyModifier,
                         Data::Effects::EffectRuntime *effectRuntime);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_DAMAGE_H

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

#ifndef GOLDBOX_COMBAT_COMBAT_AIM_H
#define GOLDBOX_COMBAT_COMBAT_AIM_H

#include "common/scummsys.h"
#include "goldbox/core/coords.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data

namespace Combat {

class CombatSession;

/**
 * Parameters of one COMBAT_AttackTarget run.
 * weaponMode: attack_mode, a weapon attack (Aim) instead of a spell pick.
 * groundOk:   target_mode, an empty cell may be picked (burst spells).
 * range:      0xFF means the readied weapon's range - 1.
 */
struct AimRequest {
    Data::PlayerCharacter *attacker;
    uint8 range;
    bool weaponMode;
    bool groundOk;

    AimRequest() : attacker(nullptr), range(0xFF), weaponMode(true),
        groundOk(false) {}
};

/** gbAimResult: the committed pick (target is null for a bare cell). */
struct AimPick {
    Data::PlayerCharacter *target;
    TilePos tile;

    AimPick() : target(nullptr), tile(0, 0) {}
};

/** The effective range of an aim run (a_range in COMBAT_AttackTarget). */
uint8 resolveAimRange(const Data::PlayerCharacter *attacker, uint8 range);

/**
 * Ranged gate shared by the Aim menu and the cursor: a ranged weapon needs
 * damaging ammunition, and either no adjacent enemy or a throwable weapon.
 * True when the attacker has no ranged weapon at all.
 */
bool rangedAttackAllowed(CombatSession &session,
                         Data::PlayerCharacter *attacker);

/** COMBAT_TargetCommandMenu: whether Target is offered for this target. */
bool aimTargetOffered(CombatSession &session, Data::PlayerCharacter *attacker,
                      Data::PlayerCharacter *target, uint8 range,
                      bool weaponMode);

/** One evaluation of the free aim cursor (body of COMBAT_manualAim). */
struct AimCell {
    bool lineOfSight;
    bool valid;
    uint8 range;
    Data::PlayerCharacter *occupant;  // combatant or downed member

    AimCell() : lineOfSight(false), valid(false), range(0xFF),
        occupant(nullptr) {}
};

AimCell evaluateAimCell(CombatSession &session, const AimRequest &request,
                        uint8 range, TilePos cell);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_AIM_H

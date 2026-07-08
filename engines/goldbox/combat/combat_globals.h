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

#ifndef GOLDBOX_COMBAT_COMBAT_GLOBALS_H
#define GOLDBOX_COMBAT_COMBAT_GLOBALS_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Combat {

/**
 * Global combat state reset at the start of each encounter.
 *
 * Mirrors the scattered globals zeroed in COMBAT_Setup:
 *   COMBAT_MAGIC_ENABLED, BYTE_COMFLAG_SLOW, COMBAT_ATTACKROLL,
 *   C_MEMBERS_ON_GROUND, PTR_CLOUD_EFF_HANDLER, PTR_USED_ITEM,
 *   VMBANK1_PARTY_STATE->D_unknownCombatFlag1
 */
struct CombatGlobals {
    bool magicEnabled;       // COMBAT_MAGIC_ENABLED
    bool slowMode;           // BYTE_COMFLAG_SLOW
    uint8 attackRoll;        // COMBAT_ATTACKROLL
    uint8 membersOnGround;   // C_MEMBERS_ON_GROUND
    bool cloudEffectActive;  // PTR_CLOUD_EFF_HANDLER != 0
    uint8 combatFlag1;       // D_unknownCombatFlag1

    CombatGlobals() { reset(); }

    void reset() {
        magicEnabled = false;
        slowMode = false;
        attackRoll = 0;
        membersOnGround = 0;
        cloudEffectActive = false;
        combatFlag1 = 0;
    }
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_GLOBALS_H

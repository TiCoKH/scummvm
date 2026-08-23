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
#include "common/array.h"
#include "goldbox/combat/cloud_effect_manager.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

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
    /**
     * Bitmask of the current hit's damage type. Set by the attacker before
     * effect handlers run; handlers check it to decide immunity/resistance.
     * Mirrors EFFECT_BEHAVE_FLAG in the original m68k code.
     */
    enum DamageTypeMask {
        DMG_FIRE         = 0x01,
        DMG_COLD         = 0x02,
        DMG_ELECTRICITY  = 0x04,
        DMG_MAGIC        = 0x08,
        DMG_ACID         = 0x10,
        DMG_DRAGON_BREATH = 0x20,
        DMG_UNKNOWN_40   = 0x40
    };

    bool magicEnabled;       // COMBAT_MAGIC_ENABLED
    bool slowMode;           // BYTE_COMFLAG_SLOW
    uint8 attackRoll;        // COMBAT_ATTACKROLL
    uint8 damage;            // BYTE_DAMAGE
    int8 moraleModifier;     // COMBAT_MORALE_MOD
    uint8 behaviorFlags;     // bitmask of DamageTypeMask
    uint8 activeSpellId;     // BYTE_SPELL_INPORCESS
    uint8 membersOnGround;   // C_MEMBERS_ON_GROUND
    uint8 combatFlag1;       // D_unknownCombatFlag1
    uint8 sideCount[2];      // ARRAY_HOSTILITY[CS_PARTY/CS_ENEMY]
    int8 savingThrow;        // SAVING_THROW per-hit modifier
    Data::PlayerCharacter *attacker; // PTR_SELECTED_CHAR — current attacking character
    bool targetUnavailable;   // Target cannot be selected
    uint8 attackCount;        // BYTE_ATTACK_COUNT : resolved attacks this turn
    bool attackCountAdjusting;
    uint8 attacksLeft;        // COMBAT_ATTACKS_LEFT — attacker's remaining attacks this turn
    uint8 turnCounter;        // COMBAT_TURN_COUNTER — incremented once at the end of each turn
    CloudEffectManager clouds; // PTR_CLOUD_EFF_HANDLER


    CombatGlobals() { reset(); }

    void reset() {
        magicEnabled = false;
        slowMode = false;
        attackRoll = 0;
        damage = 0;
        moraleModifier = 0;
        behaviorFlags = 0;
        activeSpellId = 0;
        membersOnGround = 0;
        combatFlag1 = 0;
        sideCount[0] = 0;
        sideCount[1] = 0;
        savingThrow = 0;
        attacker = nullptr;
        targetUnavailable = false;
        attackCount = 0;
        attacksLeft = 0;
        turnCounter = 0;
        clouds.reset();
    }

    void updateSideCount(const Common::Array<Data::PlayerCharacter *> &roster);
};

/**
 * Damage modifier applied to base damage before dealing it.
 * Mirrors the original DAMAGE_NULLIFY / DAMAGE_HALF constants.
 */
enum DamageModifier : uint8 {
    DAMAGE_NORMAL  = 0,
    DAMAGE_NULLIFY = 1,
    DAMAGE_HALF    = 2
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_GLOBALS_H

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

#include "goldbox/data/player_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Data {

namespace {

static const uint8 kStatusEffects[] = {
    7, 11, 30, 31, 32, 51, 52, 53, 54, 58, 59, 95, 98, 137, 74, 75
};

} // namespace

PlayerCharacter::~PlayerCharacter() {
    delete combatState;
    combatState = nullptr;
}

void PlayerCharacter::damage(uint8 amount) {
    const uint8 currentHp = hitPoints.current;
    uint8 remainingHp = 0;
    uint8 damageOverkill = 0;

    if (currentHp < amount) {
        damageOverkill = amount - currentHp;
        remainingHp = 0;
    } else {
        remainingHp = currentHp - amount;
    }

    // Legacy status transitions:
    // - <10 overkill normally avoids immediate death.
    // - exact zero HP normally becomes unconscious.
    // - animated at zero HP dies immediately (special case).
    if ((damageOverkill < 10) &&
        (remainingHp != 0 || healthStatus != S_ANIMATED)) {
        if (damageOverkill == 0) {
            if (remainingHp == 0)
                healthStatus = S_UNCONSCIOUS;
        } else {
            healthStatus = S_DYING;
            // Original game gated this with a global combat-state check
            // (BYTE_GAME_STATE == GS_COMBAT). In this engine, combatState
            // is allocated only during combat and null otherwise.
            if (combatState)
                combatState->bleeding = damageOverkill;
        }
    } else {
        healthStatus = S_DEAD;
    }

    // Only OKAY/ANIMATED remain active with remaining HP.
    if (healthStatus == S_OKAY || healthStatus == S_ANIMATED) {
        hitPoints.current = remainingHp;
        return;
    }

    // Inactive state handling.
    enabled = false;
    hitPoints.current = 0;
    // Legacy equivalent of the same global combat-state check; delay is a
    // combat-only field, so guard with combatState lifetime instead.
    if (combatState)
        combatState->delay = 0;
}

void PlayerCharacter::clearStatusEffects() {
    Effects::CharacterEffects *fx = getEffects();
    if (!fx)
        return;

    for (uint i = 0; i < ARRAYSIZE(kStatusEffects); ++i)
        fx->eraseEffectById(kStatusEffects[i]);
}

void PlayerCharacter::heal(uint8 amount) {
    hitPoints.current = MIN<uint8>(hitPoints.max, hitPoints.current + amount);
}

bool PlayerCharacter::canReceiveHealing() const {
    switch (healthStatus) {
    case S_OKAY:
    case S_ANIMATED:
    case S_UNCONSCIOUS:
    case S_DYING:
        return true;
    default:
        return false;
    }
}

bool PlayerCharacter::healHp(uint8 amount, bool normalHealing) {
    if (!canReceiveHealing())
        return false;

    if (normalHealing) {
        if (hitPoints.current >= hitPoints.max)
            return false;
    } else {
        Effects::CharacterEffects *fx = getEffects();
        if (fx && fx->hasEffect(Effects::E_POOLRAD_ENDLESS_REGEN))
            return false;
    }

    hitPoints.current = MIN<uint8>(hitPoints.max, hitPoints.current + amount);

    if (!enabled) {
        if (healthStatus == S_DYING)
            healthStatus = S_UNCONSCIOUS;

        if (healthStatus == S_UNCONSCIOUS && !combatState)
            removeUnconsciousEffect();
    }

    return true;
}

bool PlayerCharacter::isAlive() const {
    return hitPoints.current > 0;
}

void PlayerCharacter::resetCombatAction() {
    if (!combatState)
        return;
    combatState->delay = 0;
    combatState->spellId = 0;
    combatState->guarding = false;
    combatState->movePoints = 0;
}

int8 PlayerCharacter::getReactionAdjustment() const {
    uint8 dex = abilities.dexterity.current;
    if (dex <= 3)  return -4;
    if (dex <= 14) return static_cast<int8>(dex - 7);
    if (dex <= 18) return static_cast<int8>(dex - 14);
    if (dex <= 20) return 4;
    if (dex <= 23) return 5;
    if (dex <= 25) return 6;
    return 0;
}

uint8 PlayerCharacter::getStrengthTier() const {
    uint8 str_current = abilities.strength.current;
    uint8 str_exc_current = abilities.strException.current;
    if (str_current < 18) {
        return str_current;
    }
    if (str_current == 18) {
        if (str_exc_current == 0)   return 18;
        if (str_exc_current <= 50)  return 19;
        if (str_exc_current <= 75)  return 20;
        if (str_exc_current <= 90)  return 21;
        if (str_exc_current <= 99)  return 22;
        return 23; // 18/00
    }
    if (str_current < 26)
        return str_current + 5;
    return str_current; // >= 26: outside normal range, return raw
}

int8 PlayerCharacter::getStrengthBonus() const {
    if (strengthBonusAllowed == 0) return 0;

    uint8 tier = getStrengthTier();

    if (tier <= 3)  return -3;
    if (tier <= 5)  return -2;
    if (tier <= 7)  return -1;
    if (tier <= 16) return 0;
    if (tier <= 19) return 1;
    if (tier <= 22) return 2;
    if (tier <= 25) return 3;
    if (tier <= 27) return 4;
    if (tier <= 30) return static_cast<int8>(tier - 23);
    return 0;
}

int8 PlayerCharacter::getMeleeDamageBonus() const {
    if (strengthBonusAllowed == 0) return 0;

    uint8 tier = getStrengthTier();

    if (tier <= 2) return -2;
    if (tier <= 5) return -1;
    if (tier == 16) return 1;
    if (tier >= 17 && tier <= 19) return tier - 16;
    if (tier >= 20 && tier <= 29) return tier - 17;
    if (tier == 30) return 14;

    return 0;
}

int8 PlayerCharacter::getDexDefenceBonus() const {
    uint8 dex = abilities.dexterity.current;
    if (dex <= 2)  return -4;
    if (dex <= 5)  return static_cast<int8>(dex - 6);
    if (dex <= 15) return 0;
    if (dex <= 18) return static_cast<int8>(dex - 15);
    if (dex <= 20) return 3;
    if (dex <= 23) return 4;
    if (dex <= 25) return 5;
    return 0;
}

int8 PlayerCharacter::getDexSpeedBonus() const {
    uint8 dex = abilities.dexterity.current;
    if (dex < 3) return -4;
    if (dex <= 5) return static_cast<int8>(dex - 6);
    if (dex <= 15) return 0;
    if (dex <= 18) return static_cast<int8>(dex - 15);
    if (dex <= 20) return 3;
    if (dex <= 23) return 4;
    if (dex <= 25) return 5;
    return 0;
}

bool PlayerCharacter::applyStrengthChange(uint8 newStr, uint8 newExtStr, uint8 &outEncoded) {
    const uint8 oldStr    = abilities.strength.current;
    const uint8 oldExtStr = abilities.strException.current;

    // Debuff: new strength is lower
    if (newStr < oldStr || (newStr == 18 && oldStr == 18 && newExtStr < oldExtStr)) {
        outEncoded = Effects::strengthEncode(newStr, newExtStr) & 0x7f;
        return false;
    }

    // Buff: find first active strength/enlarge effect to store the backup
    Effects::CharacterEffects *fx = getEffects();
    if (fx) {
        Common::List<Effects::Effect> &list = fx->effects();
        for (Common::List<Effects::Effect>::iterator it = list.begin();
                it != list.end(); ++it) {
            Effects::Effect &e = *it;
            if ((e.id == Effects::E_STRENGTH || e.id == Effects::E_ENLARGE)
                    && e.power < 0x80) {
                e.power = Effects::strengthEncode(oldStr, oldExtStr) | 0x80;
                break;
            }
        }
    }

    abilities.strength.current    = newStr;
    abilities.strException.current = newExtStr;
    onEffectsChanged();

    outEncoded = Effects::strengthEncode(newStr, newExtStr);
    return true;
}

} // namespace Data
} // namespace Goldbox

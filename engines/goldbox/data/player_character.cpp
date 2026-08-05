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

namespace Goldbox {
namespace Data {

PlayerCharacter::~PlayerCharacter() {
    delete combatState;
    combatState = nullptr;
}

void PlayerCharacter::damage(uint8 amount) {
    if (amount >= hitPoints.current)
        hitPoints.current = 0;
    else
        hitPoints.current -= amount;
}

void PlayerCharacter::heal(uint8 amount) {
    hitPoints.current = MIN<uint8>(hitPoints.max, hitPoints.current + amount);
}

bool PlayerCharacter::isAlive() const {
    return hitPoints.current > 0;
}

int8 PlayerCharacter::getReactionAdjustment() const {
    uint8 dex = abilities.dexterity.current;
    if (dex <= 3) return -4;
    if (dex <= 6) return dex - 7;
    if (dex <= 14) return 0;
    if (dex <= 18) return dex - 14;
    if (dex == 19) return 6;
    return 0;
}

uint8 PlayerCharacter::getStrengthTier() const {
    uint8 str_current = abilities.strength.current;
    uint8 str_exc_current = abilities.strException.current;
    if (str_current < 18) {
        return str_current; // Simple case, raw strength value
    }
    if (str_current == 18) {
        if (str_exc_current == 0) return 18;
        if (str_exc_current <= 50) return 19;     // 18/01–50
        if (str_exc_current <= 75) return 20;     // 18/51–75
        if (str_exc_current <= 90) return 21;     // 18/76–90
        if (str_exc_current <= 99) return 22;     // 18/91–99
        if (str_exc_current == 100) return 23;    // 18/00
    }
    if (str_current >= 19 && str_current <= 25) {
        return str_current + 5; // 19–25 → 24–30
    }
    return 0; // Invalid or unsupported
}

int8 PlayerCharacter::getStrengthBonus() const {
    if (strengthBonusAllowed == 0) return 0;

    uint8 tier = getStrengthTier();

    if (tier <= 3) return -3;
    if (tier <= 5) return -2;
    if (tier <= 7) return -1;
    if (tier <= 17) return 0;
    if (tier <= 19) return 1;
    if (tier <= 22) return 2;
    if (tier <= 24) return 3;
    if (tier <= 26) return 4;
    if (tier >= 27 && tier <= 30) return tier - 23;

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
    if (dex < 3) return -4;
    if (dex <= 5) return static_cast<int8>(dex - 6);
    if (dex <= 14) return 0;
    if (dex <= 17) return static_cast<int8>(dex - 14);
    if (dex == 18) return 4;
    if (dex == 19) return 5;
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
            if ((e.type == Effects::E_STRENGTH || e.type == Effects::E_ENLARGE)
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

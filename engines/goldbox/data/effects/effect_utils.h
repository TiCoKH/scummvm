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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_UTILS_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_UTILS_H

#include "common/scummsys.h"
#include "common/str.h"
#include "goldbox/combat/combat_globals.h"

namespace Goldbox {
namespace Data {
namespace Effects {

/**
 * Shared formatting helpers for effect-driven damage messages.
 *
 * Keeps the presentation logic close to the effect runtime layer so combat
 * and in-game views can reuse the same wording without depending on a view-
 * specific utility.
 */
class EffectUtils {
public:
    static Common::String buildDamageMessage(uint8 finalDamage,
            uint8 behaviorFlags) {
        Common::String msg;
        if (finalDamage == 1) {
            msg = "takes 1 point of damage";
        } else {
            msg = Common::String::format("takes %u points of damage",
                    (unsigned)finalDamage);
        }

        const uint8 elemFlags = behaviorFlags & 0xf7;
        if (elemFlags & Combat::CombatGlobals::DMG_FIRE) {
            msg += " from Fire";
        } else if (elemFlags & Combat::CombatGlobals::DMG_COLD) {
            msg += " from Cold";
        } else if (elemFlags & Combat::CombatGlobals::DMG_ELECTRICITY) {
            msg += " from Electricity";
        } else if (elemFlags & Combat::CombatGlobals::DMG_ACID) {
            msg += " from Acid";
        }

        const bool isMagic =
                (behaviorFlags & Combat::CombatGlobals::DMG_MAGIC) == behaviorFlags;
        if (isMagic)
            msg += " from Magic";

        return msg;
    }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_UTILS_H

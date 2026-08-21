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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_COMMON_HANDLER_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_COMMON_HANDLER_H

#include "common/scummsys.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/data/effects/effect_handler_base.h"

namespace Goldbox {
namespace Data {
namespace Effects {

// Shared runtime effect-state flags.
// These bit positions match current Poolrad behavior and are intentionally
// kept stable for cross-game reuse where semantics align.
enum CommonEffectFlags : uint32 {
    CEF_NONE = 0,
    CEF_HELD = 1 << 0,
    CEF_PARALYZED = 1 << 1,
    CEF_SLEEPING = 1 << 2,
    CEF_HELPLESS = 1 << 3,
    CEF_BLINDED = 1 << 4,
    CEF_CONFUSED = 1 << 9,
    CEF_FUMBLING = 1 << 10,
    CEF_REGEN_1 = 1 << 25,
    CEF_REGEN_3 = 1 << 26,
    CEF_POISONED = 1 << 27
};

// Register all common effect handlers into the given handler base.
// Call this from a game-specific handler's setupHandlers() before
// registering any overrides.
void setupCommonHandlers(EffectHandlerBase &base);

// Individual common handlers exposed for reuse by game-specific handler files.
void handleEnlargeStrengthened(const EffectCall &c);
void handleFriendly(const EffectCall &c);
void handlePoisonDamage(const EffectCall &c);
void handleShield(const EffectCall &c);
void handleHelpless(const EffectCall &c);
void handleBlinded(const EffectCall &c);
void handlePrayer(const EffectCall &c);
void handleAccursed(const EffectCall &c);
void handleProtNormalWeapons(const EffectCall &c);
void handleSlow(const EffectCall &c);
void handleProtectionFromEvil(const EffectCall &c);
void handleProtectionFromGood(const EffectCall &c);
void handleParalyze(const EffectCall &c);
void handleRegen3(const EffectCall &c);
void handleSpiritualHammer(const EffectCall &c);
void handleBonusVsSmall(const EffectCall &c);
void handleDwarfVsGiant(const EffectCall &c);

// Shared helper for effects granting a chance to fully avoid the current
// hit (e.g. EFFECT_rollAvoid). Returns true if the attack was avoided.
bool rollAvoid(Goldbox::Data::PlayerCharacter &targetChar,
        Combat::CombatGlobals &combat, EffectHostBridge *bridge, uint8 percent);

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_COMMON_HANDLER_H

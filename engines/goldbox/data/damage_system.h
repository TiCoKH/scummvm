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

#ifndef GOLDBOX_DATA_DAMAGE_SYSTEM_H
#define GOLDBOX_DATA_DAMAGE_SYSTEM_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Goldbox {
namespace Data {

class PlayerCharacter;

namespace Effects {
class EffectHostBridge;
} // namespace Effects

enum DamageModifier {
    DAMAGE_NORMAL = 0,
    DAMAGE_HALF,
    DAMAGE_NULLIFY
};

struct DamageRequest {
    int amount;
    bool savingThrow;
    DamageModifier modifier;
    bool applyModifier;
    uint8 behaviorFlags;

    DamageRequest()
                : amount(0), savingThrow(false), modifier(DAMAGE_NORMAL),
                    applyModifier(false), behaviorFlags(0) {
    }

        DamageRequest(int requestedAmount, bool hasSavingThrow,
                        DamageModifier dmgModifier = DAMAGE_NORMAL,
                        bool shouldApplyModifier = false,
                        uint8 damageBehaviorFlags = 0)
                : amount(requestedAmount), savingThrow(hasSavingThrow),
                    modifier(dmgModifier),
          applyModifier(shouldApplyModifier), behaviorFlags(damageBehaviorFlags) {
    }
};

struct DamageResult {
    int requested;
    int applied;

    bool resisted;
    bool killed;
    bool wentDown;
    bool interruptedSpell;
    Common::String message;
    Common::String spellLostMessage;
    Common::String downMessage;

    DamageResult()
        : requested(0), applied(0), resisted(false), killed(false),
          wentDown(false), interruptedSpell(false), message(),
          spellLostMessage(), downMessage() {
    }
};

/**
 * Shared non-UI damage application facade for effect handlers.
 *
 * Owns damage request execution and reports a compact result payload.
 * Rendering and messaging remain in view/host layers.
 */
class DamageSystem {
public:
    explicit DamageSystem(Effects::EffectHostBridge *bridge = nullptr);

    void setHostBridge(Effects::EffectHostBridge *bridge);

    DamageResult apply(PlayerCharacter &target,
            const DamageRequest &request) const;

    // Legacy bridge-friendly adapter: resolves DAMAGE_NORMAL/HALF/NULLIFY
    // then applies through the unified request path.
    DamageResult applyLegacy(PlayerCharacter &target,
            uint8 baseDamage,
            DamageModifier modifier,
            bool applyModifier,
            uint8 behaviorFlags = 0) const;

private:
    Effects::EffectHostBridge *_bridge;
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_DAMAGE_SYSTEM_H
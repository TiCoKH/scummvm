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

#ifndef GOLDBOX_POOLRAD_EFFECT_HANDLER_H
#define GOLDBOX_POOLRAD_EFFECT_HANDLER_H

#include "goldbox/data/effects/effect_handler_base.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}

/**
 * Mirrors UTIL_CheckCloudEffect.
 * Checks whether ch is standing on a stinking-cloud tile and applies the
 * appropriate nausea effect if no immunity or existing cloud effect is present.
 * bridge may be nullptr (no message output).
 */
void checkCloudEffect(Goldbox::Data::PlayerCharacter &ch,
                      Goldbox::Data::Effects::EffectHandlerBase *handler,
                      Goldbox::Data::Effects::EffectHostBridge *bridge);

/**
 * Public entry to UTIL_CheckUnaffected: runs the immunity guard set, posts
 * "is Unaffected" when blocked (or when checkSave && saved), otherwise
 * refreshes or adds the effect and posts message. Returns true when the
 * effect was applied.
 */
bool checkUnaffectedEffect(Goldbox::Data::PlayerCharacter &ch,
                           const Goldbox::Data::Effects::EffectHandlerBase *handler,
                           Goldbox::Data::Effects::EffectHostBridge *bridge,
                           Goldbox::Combat::CombatGlobals *combat,
                           uint8 effectId, uint16 duration, uint8 power,
                           bool immediate, bool checkSave, bool saved,
                           const char *message);

/**
 * GB13-016 — pure decision helpers for monster special attacks that route
 * through the real attack resolver (Requirement 32). These carry no engine
 * state so the eligibility / validation / cleanup decisions are unit-testable
 * without a live CombatSession.
 */
struct MonsterSpecialAttacks {
    /**
     * Linked-hold effect ids cleared when a bite-and-hold grapple ends. The
     * ongoing-hold handler runs on RAW id 74 (Effect_ID74) — NOT the separately
     * named EFFECT_77_BiteAndHold attack effect, and NOT the E_POISON_NEG_2=74
     * enum that collides on that number. Clearing is the raw immobilize
     * (E_IMMOBILIZED=58) plus the raw bite-hold hold (74).
     */
    enum {
        kImmobilizeEffectId = 58,
        kBiteHoldRawEffectId = 74,
        kInvalidRosterIndex  = 0xff
    };

    /**
     * A stored victim roster index is dereferenceable only when it is in
     * range of the current roster. A sentinel/out-of-range index (e.g. the
     * victim left the roster) must NOT be dereferenced — this is the
     * stale-roster-access guard for the grapple handler.
     */
    static bool victimIndexInRange(uint8 victimIndex, uint rosterSize) {
        return victimIndex != (uint8)kInvalidRosterIndex &&
               (uint)victimIndex < rosterSize;
    }

    /**
     * The grapple's linked hold is cleared when the grapple ends: the victim
     * died, the attacker was disabled, or the effect was explicitly removed
     * (EFF_REMOVE). An ongoing EFF_ADD with both combatants still enabled keeps
     * the hold alive.
     */
    static bool shouldReleaseHold(bool isRemoveOp, bool attackerEnabled,
                                  bool victimEnabled) {
        return isRemoveOp || !attackerEnabled || !victimEnabled;
    }
};

class EffectHandler : public Goldbox::Data::Effects::EffectHandlerBase {
public:
    EffectHandler();

    void setupHandlers() override;
    Goldbox::Data::Effects::Effects mapRawEffectId(uint8 rawId) const override;

private:
    static void handleNoop(const Goldbox::Data::Effects::EffectCall &call);
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_EFFECT_HANDLER_H

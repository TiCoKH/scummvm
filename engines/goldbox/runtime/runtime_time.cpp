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

#include "goldbox/runtime/runtime_time.h"

#include "common/list.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/data/player_character.h"
#include "goldbox/ecl/ecl_memory.h"

namespace Goldbox {

namespace {

// Normalize time fields with carry propagation.
// On year overflow (field 6), increment age of all party characters.
static void timeNormalizeFields(uint16 *timeFields,
        Common::List<Data::PlayerCharacter *> &party) {
    for (int i = 0; i < kTimeFieldCount; ++i) {
        if (timeFields[i] >= kTimeFieldLimits[i]) {
            if (i == 6) {
                for (Common::List<Data::PlayerCharacter *>::iterator it = party.begin(); it != party.end(); ++it) {
                    if (*it)
                        (*it)->age++;
                }
            } else {
                timeFields[i + 1]++;
                timeFields[i] -= kTimeFieldLimits[i];
            }
        }
    }
}

// Process timed effect expiration on all party members.
// Converts (field, amount) into total base-unit ticks, then decrements
// effect durations in chunks of <=10. Expired effects are removed via
// the handler (EFF_REMOVE).
static void timeApplyTimeStepEffects(
        Common::List<Data::PlayerCharacter *> &party,
        Data::Effects::EffectHandlerBase *handler,
        GameState gameState,
        uint8 field, uint8 amount) {
    using namespace Data::Effects;

    const uint partyCount = party.size();
    if (partyCount == 0)
        return;

    // Per-character flag: still has timed effects needing processing.
    Common::Array<bool> hasActiveEffects(partyCount);

    if (gameState == GS_CAMPING) {
        bool anyActive = false;
        uint i = 0;
        for (Common::List<Data::PlayerCharacter *>::const_iterator it = party.begin(); it != party.end(); ++it, ++i) {
            CharacterEffects *fx = *it ? (*it)->getEffects() : nullptr;
            if (!fx)
                continue;
            for (uint e = 0; e < fx->effectCount(); ++e) {
                const Effect &effect = fx->effectAt(e);
                if (effect.durationMin != 0 && effect.durationMin != 0xFFFF) {
                    anyActive = true;
                    hasActiveEffects[i] = true;
                    break;
                }
            }
        }
        if (!anyActive)
            return;
    } else {
        for (uint i = 0; i < partyCount; ++i)
            hasActiveEffects[i] = true;
    }

    // Convert (field, amount) to total ticks in base time units.
    uint16 totalTicks = static_cast<uint16>(amount);
    for (int f = field; f > 1; --f) {
        totalTicks = totalTicks * kTimeFieldLimits[f - 1];
    }

    // Process in chunks of at most 10.
    while (totalTicks > 0) {
        uint8 chunkSize = (totalTicks < 11)
            ? static_cast<uint8>(totalTicks) : 10;

        uint ci = 0;
        for (Common::List<Data::PlayerCharacter *>::iterator it = party.begin(); it != party.end(); ++it, ++ci) {
            if (!hasActiveEffects[ci] || !*it)
                continue;

            CharacterEffects *fx = (*it)->getEffects();
            if (!fx)
                continue;

            hasActiveEffects[ci] = false;

            // Tail sentinel: only process effects present at chunk start.
            const uint tailIdx = fx->isEmpty() ? 0 : (fx->effectCount() - 1);
            bool reachedTail = false;

            for (uint ei = 0; ei < fx->effectCount();) {
                if (reachedTail && ei > tailIdx)
                    break;
                if (ei == tailIdx)
                    reachedTail = true;

                Effect &effect = fx->effectAt(ei);

                // Permanent (0xFFFF) or zero-duration: skip.
                if (effect.durationMin == 0 || effect.durationMin == 0xFFFF) {
                    ++ei;
                    continue;
                }

                if (static_cast<uint16>(chunkSize) < effect.durationMin) {
                    effect.durationMin -= chunkSize;
                    hasActiveEffects[ci] = true;
                    ++ei;
                } else {
                    // Expired: remove.
                    if (handler)
                        handler->apply(EFF_REMOVE, effect, **it);
                    (*it)->onEffectsChanged();
                    fx->removeEffectAt(ei);
                }
            }

            // Check remaining effects for timed entries.
            if (!hasActiveEffects[ci]) {
                for (uint ei = 0; ei < fx->effectCount(); ++ei) {
                    const Effect &effect = fx->effectAt(ei);
                    if (effect.durationMin != 0 &&
                            effect.durationMin != 0xFFFF) {
                        hasActiveEffects[ci] = true;
                        break;
                    }
                }
            }
        }

        if (totalTicks < 11)
            totalTicks = 0;
        else
            totalTicks -= 10;
    }
}

} // anonymous namespace

void timeAddUnits(ECL::AddressSpace &mem, const TimeFieldAddresses &clockAddrs,
                  Common::List<Data::PlayerCharacter *> &party,
                  Data::Effects::EffectHandlerBase *handler,
                  GameState gameState,
                  uint8 field, uint8 amount) {
    // Read 7 time fields from VM memory (byte-width, values < 256).
    uint16 timeFields[kTimeFieldCount];
    for (int i = 0; i < kTimeFieldCount; ++i) {
        timeFields[i] = mem.read8(clockAddrs.addr[i]);
    }

    // Increment the specified field 'amount' times, normalizing after each.
    for (uint8 a = 0; a < amount; ++a) {
        timeFields[field]++;
        timeNormalizeFields(timeFields, party);
    }

    // Write back to VM memory (byte-width).
    for (int i = 0; i < kTimeFieldCount; ++i) {
        mem.write8(clockAddrs.addr[i], static_cast<uint8>(timeFields[i]));
    }

    // Process timed effect expiration.
    timeApplyTimeStepEffects(party, handler, gameState, field, amount);
}

} // namespace Goldbox

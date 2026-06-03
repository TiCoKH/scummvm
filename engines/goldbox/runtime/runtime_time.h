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

#ifndef GOLDBOX_RUNTIME_RUNTIME_TIME_H
#define GOLDBOX_RUNTIME_RUNTIME_TIME_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/core/global.h"

namespace Goldbox {

namespace Data {
class PlayerCharacter;
namespace Effects {
class EffectHandlerBase;
}
}

namespace ECL {
class AddressSpace;
}

// Time field indices (7 fields total):
//   0 = units (sub-minutes)
//   1 = minutes (ones digit)
//   2 = minutes (tens digit)
//   3 = hours
//   4 = days
//   5 = months
//   6 = years
static const int kTimeFieldCount = 7;

// Maximum value for each time field before it carries into the next.
// Original data at ARRAY_TIME_FIELD_LIMITS (x86: 0x363a, m68k: DAT_002849aa).
static const uint16 kTimeFieldLimits[kTimeFieldCount] = {
    10,   // units per minute-ones
    10,   // minute-ones per minute-tens
    6,    // minute-tens per hour
    24,   // hours per day
    30,   // days per month
    12,   // months per year
    0xFFFF // years never overflow
};

/**
 * Array of 7 VM byte addresses for the clock fields, in order:
 * [units, minuteOnes, minuteTens, hour, day, month, yearLo].
 *
 * For Poolrad these are:
 *   { 0x49C5, 0x49C7, 0x49C8, 0x49C9, 0x49CA, 0x49CB, 0x49CC }
 *
 * Despite the original using word-sized storage, the ScummVM flat memory
 * model stores these as byte values (read8/write8). The values never exceed
 * 255 in practice.
 */
struct TimeFieldAddresses {
    uint16 addr[kTimeFieldCount];
};

/**
 * Advance the game clock by the given amount in the specified time field,
 * normalize (carry), age characters on year rollover, and process timed
 * effect expiration on all party members.
 *
 * This implements the original TIME_AddUnits + TIME_NormalizeFields +
 * TIME_ApplyTimeStepEffects chain.
 *
 * @param mem         ECL address space containing VM clock fields.
 * @param clockAddrs  VM addresses for the 7 clock fields.
 * @param party       Array of party character pointers.
 * @param handler     Effect handler for applying EFF_REMOVE on expiry.
 * @param gameState   Current game state (for camping check).
 * @param field       Time field index to increment (1 = minutes, 2 = tens, etc.)
 * @param amount      How many increments to apply.
 */
void timeAddUnits(ECL::AddressSpace &mem, const TimeFieldAddresses &clockAddrs,
                  Common::Array<Data::PlayerCharacter *> &party,
                  Data::Effects::EffectHandlerBase *handler,
                  GameState gameState,
                  uint8 field, uint8 amount);

} // namespace Goldbox

#endif // GOLDBOX_RUNTIME_RUNTIME_TIME_H

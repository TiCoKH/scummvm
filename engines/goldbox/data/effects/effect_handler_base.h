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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_HANDLER_BASE_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_HANDLER_BASE_H

#include "common/hashmap.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Effects {

// Effect operation type: add, remove, or tick (per turn).
//
// Semantics:
// - EFF_ADD: apply modifiers/flags for the effect once.
// - EFF_REMOVE: reverse the modifiers/flags that EFF_ADD applied.
// - EFF_TICK: periodic per-turn processing (no modifier reversal).
//
// NOTE: This assumes effects are applied/removed as a single instance.
// If stacking is needed, the runtime should either track reference counts
// or recompute aggregate modifiers from active effects each turn.
enum EffectOp {
    EFF_ADD    = 0,
    EFF_REMOVE = 1,
    EFF_TICK   = 2
};

class EffectHandlerBase {
public:
    using Handler = void (*)(EffectOp op, Effect &effect, Goldbox::Data::PlayerCharacter &character);

    virtual ~EffectHandlerBase() {}

    void apply(EffectOp op, Effect &effect, Goldbox::Data::PlayerCharacter &character) const;
    bool hasHandler(uint8 effectType) const;

protected:
    EffectHandlerBase();

    void clearHandlers();
    void setHandler(Effects effectId, Handler handler);
    void setDefaultHandler(Handler handler);

    virtual void setupHandlers() = 0;
    virtual Effects mapRawEffectId(uint8 rawId) const = 0;

private:
    Handler getHandler(uint8 effectType) const;

    Common::HashMap<uint8, Handler> _handlers;
    Handler _defaultHandler;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_HANDLER_BASE_H

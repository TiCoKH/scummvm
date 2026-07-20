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
#include "common/scummsys.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Effects {

struct EffectExecutionContext;

// Effect operation type: add, remove, or tick (per turn).
//
// Semantics:
// - EFF_ADD: apply modifiers/flags for the effect once.
// - EFF_REMOVE: reverse the modifiers/flags that EFF_ADD applied.
// - EFF_TICK: periodic per-turn processing (no modifier reversal).
// - EFF_EVAL: context-aware trigger-set evaluation.
//
// NOTE: This assumes effects are applied/removed as a single instance.
// If stacking is needed, the runtime should either track reference counts
// or recompute aggregate modifiers from active effects each turn.
enum EffectOp {
    EFF_ADD    = 0,
    EFF_REMOVE = 1,
    EFF_TICK   = 2,
    EFF_EVAL   = 3
};

struct EffectCall0 {
    EffectOp op;
    Effect &effect;
    Goldbox::Data::PlayerCharacter &character;
    const EffectExecutionContext *ctx;

    EffectCall0(EffectOp operation, Effect &effectRef,
            Goldbox::Data::PlayerCharacter &characterRef,
            const EffectExecutionContext *context = nullptr)
            : op(operation), effect(effectRef), character(characterRef),
              ctx(context) {
    }
};

struct EffectCall1 : public EffectCall0 {
    intptr_t arg0;

    EffectCall1(EffectOp operation, Effect &effectRef,
            Goldbox::Data::PlayerCharacter &characterRef,
            const EffectExecutionContext *context,
            intptr_t a0) : EffectCall0(operation, effectRef,
            characterRef, context), arg0(a0) {
    }
};

struct EffectCall2 : public EffectCall0 {
    intptr_t arg0;
    intptr_t arg1;

    EffectCall2(EffectOp operation, Effect &effectRef,
            Goldbox::Data::PlayerCharacter &characterRef,
            const EffectExecutionContext *context,
            intptr_t a0, intptr_t a1) : EffectCall0(operation, effectRef,
            characterRef, context), arg0(a0), arg1(a1) {
    }
};

struct EffectCall3 : public EffectCall0 {
    intptr_t arg0;
    intptr_t arg1;
    intptr_t arg2;

    EffectCall3(EffectOp operation, Effect &effectRef,
            Goldbox::Data::PlayerCharacter &characterRef,
            const EffectExecutionContext *context,
            intptr_t a0, intptr_t a1, intptr_t a2) : EffectCall0(operation,
            effectRef, characterRef, context), arg0(a0), arg1(a1), arg2(a2) {
    }
};

class EffectHandlerBase {
public:
    using Handler0 = void (*)(const EffectCall0 &call);
    using Handler1 = void (*)(const EffectCall1 &call);
    using Handler2 = void (*)(const EffectCall2 &call);
    using Handler3 = void (*)(const EffectCall3 &call);

    virtual ~EffectHandlerBase() {}

    void apply(EffectOp op, Effect &effect, Goldbox::Data::PlayerCharacter &character) const;
    void apply(EffectOp op, Effect &effect,
            Goldbox::Data::PlayerCharacter &character,
            const EffectExecutionContext *ctx) const;
    void apply(EffectOp op, Effect &effect,
            Goldbox::Data::PlayerCharacter &character,
            const EffectExecutionContext *ctx, int32 arg0) const;
    void apply(EffectOp op, Effect &effect,
            Goldbox::Data::PlayerCharacter &character,
            const EffectExecutionContext *ctx, int32 arg0,
            int32 arg1) const;
    void apply(EffectOp op, Effect &effect,
            Goldbox::Data::PlayerCharacter &character,
            const EffectExecutionContext *ctx, int32 arg0,
            int32 arg1, int32 arg2) const;
    void apply(const EffectCall0 &call) const;
    void apply(const EffectCall1 &call) const;
    void apply(const EffectCall2 &call) const;
    void apply(const EffectCall3 &call) const;
    bool hasHandler(uint8 effectType) const;

protected:
    EffectHandlerBase();

    void clearHandlers();
    void setHandler(Effects effectId, Handler0 handler);
    void setHandler(Effects effectId, Handler1 handler);
    void setHandler(Effects effectId, Handler2 handler);
    void setHandler(Effects effectId, Handler3 handler);
    void setDefaultHandler(Handler0 handler);

    virtual void setupHandlers() = 0;
    virtual Effects mapRawEffectId(uint8 rawId) const = 0;

private:
    Handler0 getHandler0(uint8 effectType) const;
    Handler1 getHandler1(uint8 effectType) const;
    Handler2 getHandler2(uint8 effectType) const;
    Handler3 getHandler3(uint8 effectType) const;

    Common::HashMap<uint8, Handler0> _handlers0;
    Common::HashMap<uint8, Handler1> _handlers1;
    Common::HashMap<uint8, Handler2> _handlers2;
    Common::HashMap<uint8, Handler3> _handlers3;
    Handler0 _defaultHandler;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_HANDLER_BASE_H

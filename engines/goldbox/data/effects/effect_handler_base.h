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
#include "goldbox/combat/combat_globals.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Items {
struct CharacterItem;
}

namespace Effects {

class EffectHandlerBase;

// Effect operation type.
enum EffectOp {
    EFF_ADD    = 0,
    EFF_REMOVE = 1,
    EFF_TICK   = 2,
    EFF_EVAL   = 3
};

// Single call record passed to every handler.
// combat is null outside of combat (tick, non-combat spell application).
// item is set for item ready/unready handlers, which act on the item
// itself; effect then holds a copy of its effect bytes.
struct EffectCall {
    EffectOp op;
    Effect &effect;
    Goldbox::Data::PlayerCharacter &character;
    Combat::CombatGlobals *combat;
    EffectHostBridge *bridge;
    DamageSystem *damage;
    const EffectHandlerBase *handler;
    Items::CharacterItem *item;

    EffectCall(EffectOp operation, Effect &effectRef,
            Goldbox::Data::PlayerCharacter &characterRef,
            Combat::CombatGlobals *combatGlobals = nullptr,
            EffectHostBridge *hostBridge = nullptr,
            DamageSystem *damageSystem = nullptr,
            const EffectHandlerBase *handlerRef = nullptr,
            Items::CharacterItem *itemRef = nullptr)
            : op(operation), effect(effectRef), character(characterRef),
              combat(combatGlobals), bridge(hostBridge),
              damage(damageSystem), handler(handlerRef), item(itemRef) {
    }
};

class EffectHandlerBase {
public:
    using Handler = void (*)(const EffectCall &call);

    virtual ~EffectHandlerBase();

    void apply(EffectOp op, Effect &effect,
            Goldbox::Data::PlayerCharacter &character,
            Combat::CombatGlobals *combat = nullptr) const;
    void apply(EffectOp op, Effect &effect,
            Goldbox::Data::PlayerCharacter &character,
            Combat::CombatGlobals *combat,
            EffectHostBridge *bridge) const;
    void apply(const EffectCall &call) const;
    // Run effectId's handler on call, whatever call.effect.id holds
    // (FxEngine.CallEffectHandler with another effect's node).
    void dispatch(uint8 effectId, const EffectCall &call) const;
    bool hasHandler(uint8 effectType) const;

    // Raw effect ids checked by trigger set setId, in dispatch order.
    // Returns nullptr when the game uses the runtime's built-in table.
    virtual const uint8 *getTriggerSet(uint8 setId, uint &count) const;

    // Aura effects that also apply to creatures near the carrier. range is
    // the combat radius in tiles.
    virtual bool isGroupRadiatingEffect(uint8 rawId, uint8 &range) const;

    // Effects removed when a creature drops to 0 HP or is put out of play.
    virtual const uint8 *getHpZeroEffects(uint &count) const;

protected:
    EffectHandlerBase();

    void clearHandlers();
    void setHandler(Effects effectId, Handler handler);
    void setSpecHandler(uint8 rawId, Handler handler);
    void setDefaultHandler(Handler handler);

    virtual void setupHandlers() = 0;
    virtual Effects mapRawEffectId(uint8 rawId) const = 0;

    friend void setupCommonHandlers(EffectHandlerBase &base);

private:
    Handler getHandler(uint8 effectType) const;
    Handler getRawHandler(uint8 rawId) const;

    Common::HashMap<uint8, Handler> _handlers;
    Common::HashMap<uint8, Handler> _rawHandlers;
    Handler _defaultHandler;
};

extern EffectHandlerBase *g_effectHandler;

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_HANDLER_BASE_H

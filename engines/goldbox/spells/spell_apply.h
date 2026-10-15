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

#ifndef GOLDBOX_SPELLS_SPELL_APPLY_H
#define GOLDBOX_SPELLS_SPELL_APPLY_H

#include "common/str.h"
#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_definition.h"
#include "goldbox/spells/spell_handlers.h"

namespace Goldbox {
namespace Spells {

/**
 * Parameters of SPELL_ApplyOnTargets that vary per spell handler.
 */
struct SpellApplyParams {
    // Message posted when the effect lands; empty uses definition.effectMessage.
    Common::String message;
    // DAMAGE_BEHAVIOR flags (only used when baseDamage != 0).
    uint8 behaviorFlags;
    uint8 baseDamage;
    // effect_flag: the effect fires its immediate handler when added.
    bool effectImmediate;
    // Non-zero overrides the effect power (default: caster level).
    uint8 effectPowerOverride;

    SpellApplyParams()
        : behaviorFlags(0), baseDamage(0), effectImmediate(false),
        effectPowerOverride(0) {}
};

/**
 * Mirrors SPELL_ApplyOnTargets. For each target:
 *   1. saving throw when the spell has a saving effect;
 *   2. attack roll when the spell needs one (fixedRange == 0xFF); a miss
 *      zeroes the damage for this and every later target and counts as saved;
 *   3. damage (saving effect decides negate/half);
 *   4. effect via UTIL_CheckUnaffected semantics ("is Unaffected" when
 *      guarded, or saved for a spell with a saving effect).
 */
SpellCastResult applyOnTargets(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets,
        const SpellApplyParams &params);

/**
 * Mirrors SPELL_ApplyToTargetList: drops every target not on `side`
 * (for Bless, also targets engaged in melee) and then applies the spell's
 * saving-throw/effect path with `message`.
 */
SpellCastResult applyToSide(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets,
        Goldbox::Data::CombatSide side,
        const Common::String &message);

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_APPLY_H

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
 */

#ifndef GOLDBOX_SPELLS_SPELL_GENERIC_HANDLER_H
#define GOLDBOX_SPELLS_SPELL_GENERIC_HANDLER_H

#include "goldbox/spells/spell_handlers.h"

namespace Goldbox {
namespace Spells {

/**
 * Default spell handler used whenever a SpellDefinition has no game- or
 * spell-specific HandlerId (kHandlerGeneric).
 *
 * Mirrors the original SPELL_ApplyOnTargets generic resolution path for
 * pure status-effect spells: per target, roll a saving throw if the spell
 * allows one, then add/refresh the SpellEntry::effectId with a duration
 * computed from fixedDuration + perLvlDuration * casterLevel.
 *
 * Spells that also need an attack roll, direct damage, or other special
 * casing (e.g. Magic Missile, Fireball, Hold Person's HD limit) should be
 * registered with a dedicated HandlerId/ISpellHandler instead; this class
 * intentionally does not deal damage since SpellEntry carries no damage
 * dice data.
 */
class GenericSpellHandler : public ISpellHandler {
public:
	SpellCastResult execute(const SpellContext &context,
	                        const SpellDefinition &definition,
	                        const TargetSelection &targets) const override;

	// Shared apply path used by specialised handlers that need a non-default
	// effect power. effectPowerOverride == 0 -> use casterLevel as power.
	static SpellCastResult applyToTargets(const SpellContext &context,
	                                      const SpellDefinition &definition,
	                                      const TargetSelection &targets,
	                                      uint8 effectPowerOverride);
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_GENERIC_HANDLER_H
